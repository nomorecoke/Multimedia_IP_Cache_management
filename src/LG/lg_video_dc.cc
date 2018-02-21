#include "LG/lg_video_dc.hh"
#include "params/LGVideoDisplay.hh"
#include "base/intmath.hh"

#include <iostream>
#include <string>

#define GOT_HERE std::cout << "At " __FILE__":" << __LINE__ << std::endl;

LGVideoDisplay::LGVideoDisplay(const LGVideoDisplayParams* params)
    : MemObject(params),
      IPResponseQueue(*params->response_queue),
      tickEvent(*this),
      processing_cycles(params->processing_latency),
      full_frame_size(params->full_frame_size),
      encoded_frame_size(params->encoded_frame_size),
      input_buffer_size(params->input_buffer_size),
      output_buffer_size(params->output_buffer_size),
      coding_ratio(params->coding_ratio),
      read_port(this, params->system)
{
    
    idle_cycles = 0;
    active_cycles = 0;
    data_stall_cycles = 0;
    read_stall_cycles = 0;
    write_stall_cycles = 0;
    input_buffer_full = 0;
    output_buffer_empty = 0;
    true_processing_cycles = 0;

    IPRequestQueue.clear();  
    pktPtr = nullptr;
    readBufferPtr = nullptr;
    processBufferPtr = nullptr;
    writeBufferPtr = nullptr;

    // TODO: chunk size parameter
    buffer_size = 0x40; // 0x400; // subframe size fixed
    
    inputBufferQueue = new BufferQueue(buffer_size, input_buffer_size);
    outputBufferQueue = new BufferQueue(buffer_size, output_buffer_size);
/*
    read_dma = new DmaFifo(read_port, 0x400, 64, 256, 0x0);
    write_dma = new DmaFifo(write_port, 0x400, 64, 256, 0x0);
  */

    int reqs_per_buffers = divCeil(buffer_size, 64);
    int max_dma_req = divCeil(256, reqs_per_buffers);
    
    read_dma = new LGDma(read_port, max_dma_req, inputBufferQueue, "dc_read");
    //write_dma = new LGDma(write_port, max_dma_req, outputBufferQueue);

    isBusy = false;


    Initialize();
}

LGVideoDisplay::~LGVideoDisplay()
{
    delete read_dma;
//    delete write_dma;
    delete inputBufferQueue;
    delete outputBufferQueue;
}

void
LGVideoDisplay::Initialize()
{
    schedule(tickEvent, 500000000000);
}

BaseMasterPort&
LGVideoDisplay::getMasterPort(const std::string& if_name,
        PortID idx)
{
    if(if_name == "read_port")
        return read_port;
 //   else if(if_name == "write_port")
 //       return write_port;
    else
        return MemObject::getMasterPort(if_name, idx);
}

void
LGVideoDisplay::tick()
{
    if(isBusy) { // Active state
        bool load_flag = frameLoad();
        bool process_flag = frameProcess();
        bool store_flag = frameStore();
        if(load_flag && process_flag && store_flag) {
            pktPtr->comm = IP_RESPONSE;
            IPResponseQueue.push_back(pktPtr);
            DPRINTF(LGVideoDisplay, "vid_dc finish the processing(%d)\n", pktPtr->frame_num);
            pktPtr = nullptr;
            isBusy = false;
            IPResponseQueue.dcBusy = false;
        }
    }
    else { // Idle state
        idle_cycles++;

        if(!IPRequestQueue.empty()) {
            pktPtr = IPRequestQueue.front(); IPRequestQueue.pop_front();
            read_start_addr = pktPtr->read_addr;
            read_cur_addr = pktPtr->read_addr;
            read_last_addr = pktPtr->read_addr + full_frame_size;
            
            /**/
            process_start_addr = read_start_addr;
            process_last_addr = read_last_addr;
            process_cur_addr = read_cur_addr;

            isBusy = true;
            IPResponseQueue.dcBusy = true;

            DPRINTF(LGVideoDisplay, "vid_dc get a signal(%d)\n", pktPtr->frame_num);
        }
    }

    schedule(tickEvent, clockEdge(Cycles(1)));
}

bool
LGVideoDisplay::frameLoad()
{
    if (read_cur_addr >= read_last_addr)
        return true;

    if (!readBufferPtr) { 
        if ((readBufferPtr = inputBufferQueue->getFreeBuffer())) {
            // Make a new buffer reuqest
            readBufferPtr->state = LOAD;
            readBufferPtr->start_addr = read_cur_addr;
            readBufferPtr->buffer_size = buffer_size;
            readBufferPtr->load_latency = curTick();
        }
        else
        {
            /* Memstall due to lack of input buffer */
            input_buffer_full++;
            return false;
        }
    }
#if 1
// For fixed address mapping 
  if (read_cur_addr - read_start_addr <= FIXED_RANGE)
//    if (read_last_addr - read_cur_addr < FIXED_RANGE)
    {
        if(read_dma->issueDmaRequest(MemCmd::ReadReq, read_cur_addr, buffer_size, readBufferPtr, true))
        {
            readBufferPtr = nullptr;
            read_cur_addr += buffer_size;
        }

        return false;
    }
#else
    // for fixed address mapping with stride 6
    int buffCount = (read_cur_addr - read_start_addr) / buffer_size;
    if (buffCount % 6 == 0)
    {
        if(read_dma->issueDmaRequest(MemCmd::ReadReq, read_cur_addr, buffer_size, readBufferPtr, true))
        {
            readBufferPtr = nullptr;
            read_cur_addr += buffer_size;
        }

        return false;
    }
#endif

    if ((read_dma->issueDmaRequest(MemCmd::ReadReq, read_cur_addr, buffer_size, readBufferPtr)))
    {
        readBufferPtr = nullptr;
        read_cur_addr += buffer_size;
    }
    
    return false;
}

bool
LGVideoDisplay::frameProcess()
{
    /* Update State */
    active_cycles++;

    /* Case 1: Processing for full frame is over */
    if (processBufferPtr == nullptr 
		&& process_cur_addr >= process_last_addr
		&& inputBufferQueue->usedBuffer_num == 0) {
        true_processing_cycles++;
        return true;
    }
    
    /* Case 2: Previous buffer processing is on going */
    if (processBufferPtr) {
        /* Case 2-1: Wait for processing */
        if (processBufferPtr->process_latency < (processing_cycles * buffer_size) / 64) {
            processBufferPtr->process_latency++;
            true_processing_cycles++;
            return false;
        }
        /* Case 2-2: Previous buffer processing has done, Now generate new buffer requests for storing */
        else {
            /* Case 2-2-1: Generating buffer requests has done, release the input buffer */
            process_cur_addr += buffer_size;
            processBufferPtr->process_latency = 0;
            inputBufferQueue->releaseBuffer(processBufferPtr);
            processBufferPtr = nullptr;
            true_processing_cycles++;
            return false;
        }
    }

    /* Case 3: Receive a loaded buffer from input Buffer */
    if ((processBufferPtr = inputBufferQueue->getUsedBuffer())) {
        processBufferPtr->state = PROCESS;
        process_mark_addr = process_cur_addr + buffer_size;
        return false;
    }
    /* Case 4: There is no available input buffer for processing */
    else {
        read_stall_cycles++;
        return false;
    }
}

bool
LGVideoDisplay::frameStore()
{
	return true;
}

void
LGVideoDisplay::regStats()
{
    MemObject::regStats();
    idle_cycles.name(name() + ".idle_cycles")
        .desc("Number of idle cycles")
        ;
    active_cycles.name(name() + ".active_cycles")
        .desc("Number of active cycles")
        ;
    data_stall_cycles.name(name() + ".data_stall_cycles")
        .desc("Number of under running cycles")
        ;
    read_stall_cycles.name(name() + ".read_stall_cycles")
        .desc("Number of under running cycles")
        ;
    write_stall_cycles.name(name() + ".write_stall_cycles")
        .desc("Number of under running cycles")
        ;
    input_buffer_full.name(name() + ".input_buffer_full")
        .desc("Number of under running cycles")
        ;
    output_buffer_empty.name(name() + ".output_buffer_empty")
        .desc("Number of under running cycles")
        ;
    true_processing_cycles.name(name() + ".true_processing_cycles")
        .desc("Number of under running cycles")
        ;
}

void
LGVideoDisplay::resetStats()
{
    MemObject::resetStats();
}

LGVideoDisplay*
LGVideoDisplayParams::create()
{
    return new LGVideoDisplay(this);
}
