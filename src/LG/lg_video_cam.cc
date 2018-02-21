#include "LG/lg_video_cam.hh"
#include "params/LGVideoCamera.hh"
#include "base/intmath.hh"

#include <iostream>
#include <string>

#define GOT_HERE std::cout << "At " __FILE__":" << __LINE__ << std::endl;

LGVideoCamera::LGVideoCamera(const LGVideoCameraParams* params)
    : MemObject(params),
      IPResponseQueue(*params->response_queue),
      tickEvent(*this),
      processing_cycles(params->processing_latency),
      full_frame_size(params->full_frame_size),
      encoded_frame_size(params->encoded_frame_size),
      input_buffer_size(params->input_buffer_size),
      output_buffer_size(params->output_buffer_size),
      coding_ratio(params->coding_ratio),
      write_port(this, params->system)
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
    buffer_size = 0x400; // subframe size fixed
    
    inputBufferQueue = new BufferQueue(buffer_size, input_buffer_size);
    outputBufferQueue = new BufferQueue(buffer_size, output_buffer_size);
/*
    read_dma = new DmaFifo(read_port, 0x400, 64, 256, 0x0);
    write_dma = new DmaFifo(write_port, 0x400, 64, 256, 0x0);
  */

    int reqs_per_buffers = divCeil(buffer_size, 64);
    int max_dma_req = divCeil(1028, reqs_per_buffers);
    
 //   read_dma = new LGDma(read_port, max_dma_req, inputBufferQueue);
    write_dma = new LGDma(write_port, max_dma_req, outputBufferQueue, "cam_write");

    isBusy = false;

    Initialize();
}

LGVideoCamera::~LGVideoCamera()
{
 //   delete read_dma;
    delete write_dma;
    delete inputBufferQueue;
    delete outputBufferQueue;
}

void
LGVideoCamera::Initialize()
{
    schedule(tickEvent, 500000000000);
}

BaseMasterPort&
LGVideoCamera::getMasterPort(const std::string& if_name,
        PortID idx)
{
	/*
    if(if_name == "read_port")
        return read_port;
    else */if(if_name == "write_port")
        return write_port;
    else
        return MemObject::getMasterPort(if_name, idx);
}

void
LGVideoCamera::tick()
{
    if(isBusy) { // Active state
        bool load_flag = frameLoad();
        bool process_flag = frameProcess();
        bool store_flag = frameStore();
        if(load_flag && process_flag && store_flag) {
            pktPtr->comm = IP_RESPONSE;
            IPResponseQueue.push_back(pktPtr);
            DPRINTF(LGVideoCamera, "vid_cam finish the processing(%d)\n", pktPtr->frame_num);
            pktPtr = nullptr;
            isBusy = false;
        }
    }
    else { // Idle state
        idle_cycles++;

        if(!IPRequestQueue.empty()) {
            pktPtr = IPRequestQueue.front(); IPRequestQueue.pop_front();
            write_start_addr = pktPtr->write_addr;
            write_cur_addr = pktPtr->write_addr;
            write_last_addr = pktPtr->write_addr + full_frame_size;

            /**/
            process_start_addr = write_start_addr;
            process_last_addr = write_last_addr;
            process_cur_addr = write_cur_addr;

            isBusy = true;

            DPRINTF(LGVideoCamera, "vid_cam get a signal(%d)\n", pktPtr->frame_num);
        }
    }

    schedule(tickEvent, clockEdge(Cycles(1)));
}

bool
LGVideoCamera::frameLoad()
{
    return true;
}

bool
LGVideoCamera::frameProcess()
{
    /* Update State */
    active_cycles++;

    /* Case 1: Processing for full frame is over */
    if (processBufferPtr == nullptr && process_cur_addr >= process_last_addr) {
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
            if (process_cur_addr >= process_mark_addr) {
                processBufferPtr->process_latency = 0;
                inputBufferQueue->releaseBuffer(processBufferPtr);
                processBufferPtr = nullptr;
                true_processing_cycles++;
                return false;
            }

            /* Case 2-2-2: Generating buffer is on going */
            VBuffer* tempBuffer = nullptr;
            if ((tempBuffer = outputBufferQueue->getFreeBuffer())) {
                // Make a new buffer reuqest
                tempBuffer->state = STORE;
                tempBuffer->start_addr = process_cur_addr;
                tempBuffer->buffer_size = buffer_size;
                
                //enqueue processed buffer to outputBufferQueue
                process_cur_addr += buffer_size;
                outputBufferQueue->enqueueBuffer(tempBuffer);
                true_processing_cycles++;
                return false;
            }
            /* Case 2-2-3: There is no available output buffer for storing */
            else {
                write_stall_cycles++;
                return false;
            }
        }
    }

    /* Case 3: Receive a loaded buffer from input Buffer */
    if ((processBufferPtr = inputBufferQueue->getFreeBuffer())) {
        processBufferPtr->state = PROCESS;
	processBufferPtr->process_latency = 0;
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
LGVideoCamera::frameStore()
{
    if (write_cur_addr >= write_last_addr && outputBufferQueue->usedBuffer_num == 0)
        return true;

    if (!writeBufferPtr) { 
        if ((writeBufferPtr = outputBufferQueue->getUsedBuffer())) {
            // Make a new buffer reuqest
            writeBufferPtr->state = STORE;
            writeBufferPtr->load_latency = curTick();
            return false;
        }
        else
        {
            /* Memstall due to lack of input buffer */
            output_buffer_empty++;
            return false;
        }
    }

    if ((write_dma->issueDmaRequest(MemCmd::WriteReq, writeBufferPtr->start_addr, buffer_size, writeBufferPtr)))
    {
        write_cur_addr = writeBufferPtr->start_addr + writeBufferPtr->buffer_size;
        writeBufferPtr = nullptr;
        return false;
    }

    return false;
}

void
LGVideoCamera::regStats()
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
LGVideoCamera::resetStats()
{
    MemObject::resetStats();
}

LGVideoCamera*
LGVideoCameraParams::create()
{
    return new LGVideoCamera(this);
}
