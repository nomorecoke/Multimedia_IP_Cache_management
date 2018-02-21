#include "LG/lg_video_nw.hh"
#include "params/LGVideoNW.hh"

LGVideoNW::LGVideoNW(const LGVideoNWParams* params)
    : MemObject(params),
      tickEvent(*this),
      buffer_size(params->buffer_size),
      full_frame_size(params->full_frame_size),
      encoded_frame_size(params->encoded_frame_size),
      setup_delay(params->setup_delay),
      processing_latency(params->processing_latency),
      sleep_latency(params->sleep_latency),
      wakeup_latency(params->wakeup_latency),
      port(this, params->system),
      IPResponseQueue(*params->response_queue)
{
    dma = new DmaFifo(port, buffer_size, 64, 256, 0x0);

    idle_cycles = 0;
    read_cycles = 0;
    processing_cycles = 0;
    write_cycles = 0;
    sleep_cycles = 0;
    active_cycles = 0;
    slack_cycles = 0;

    IPRequestQueue.clear();

    RESET_IP_TIMER(processing_timer);
    RESET_IP_TIMER(sleep_timer);
    RESET_IP_TIMER(wakeup_timer);

    state = IP_IDLE;

    pktPtr = nullptr;

//    init();
}

LGVideoNW::~LGVideoNW()
{
    delete dma;
}

BaseMasterPort&
LGVideoNW::getMasterPort(const std::string& if_name,
        PortID idx)
{
    if(if_name == "dma_port")
        return port;
    else
        return MemObject::getMasterPort(if_name, idx);
}

void
LGVideoNW::init() 
{
    schedule(tickEvent, 500000000000);
}

void
LGVideoNW::tick()
{
    bool isEnd = false;

    switch(state) {
        case IP_IDLE:
            isEnd = idle_state();
            break;
        case IP_READ:
            isEnd = read_state();
            break;
        case IP_DOWNLOAD:
		case IP_UPLOAD:
            isEnd = process_state();
            break;
        case IP_WRITE:
            isEnd = write_state();
            break;
        case IP_SLEEP:
            isEnd = sleep_state();
            break;
        default:
            panic("Unknown IP state!\n");
            break;
    }
    
    if(isEnd)
        transition();

    schedule(tickEvent, clockEdge(Cycles(1)));
}

void
LGVideoNW::transition()
{
    switch(state) {
        case IP_IDLE:
            // TODO: transition to sleep state
			if(pktPtr->type == NW_DOWNLOAD) {
				RESET_IP_TIMER(processing_timer);
				state = IP_DOWNLOAD;
                        }
			else if(pktPtr->type == NW_UPLOAD)
				state = IP_READ;
			slack_latency.sample(slack_cycles);
            break;
        case IP_READ:
            RESET_IP_TIMER(processing_timer);
            state = IP_UPLOAD;
            break;
        case IP_DOWNLOAD:
            state = IP_WRITE;
            break;
        case IP_UPLOAD:
            slack_cycles = 0;
            state = IP_IDLE;
            break;
        case IP_WRITE:
            slack_cycles = 0;
            state = IP_IDLE;
            break;
        case IP_SLEEP:
            slack_cycles = 0;
            state = IP_IDLE;
            break;
        default:
            panic("Unknown IP state!\n");
            break;
    }
}

bool
LGVideoNW::idle_state()
{
    idle_cycles++;
    slack_cycles++;

    /*
    if(slack_cycles > THRESHOLDS_CYCLES) {
        if(sleep_timer == 0) {
            SET_IP_TIMER(sleep_timer, sleep_latency);
        }
        else if(sleep_timer == 1)
        {
            return true;
        }
        else
            sleep_timer--;
            
    }
    */

    if(!IPRequestQueue.empty())
    {
        pktPtr = IPRequestQueue.front(); IPRequestQueue.pop_front();
        read_frame_addr = pktPtr->read_addr;
        write_frame_addr = pktPtr->write_addr;
        read_cur_addr = pktPtr->read_addr;
        write_cur_addr = pktPtr->write_addr;
        read_last_addr = pktPtr->read_addr + encoded_frame_size;
        write_last_addr = pktPtr->write_addr + encoded_frame_size;

        DPRINTF(LGVideoNW, "nw get a signal(%d)\n", pktPtr->frame_num);

        return true;
    }

    return false;
}

bool
LGVideoNW::read_state()
{
    read_cycles++;

    if(dma->isActive())
    {
    }
    else
    {
        if(read_frame_addr >= read_cur_addr) {
            DPRINTF(LGVideoNW, "nw start to read frame(%d)\n", pktPtr->frame_num);
            size_t chunk_size = read_last_addr > read_cur_addr + buffer_size
                ? buffer_size : read_last_addr - read_cur_addr;
            dma->flush();
            dma->startFill(read_cur_addr, chunk_size);
            read_cur_addr += chunk_size;
        }
        else if (read_cur_addr < read_last_addr) {
            size_t chunk_size = read_last_addr > read_cur_addr + buffer_size
                ? buffer_size : read_last_addr - read_cur_addr;
            dma->flush();
            dma->startFill(read_cur_addr, chunk_size);
            read_cur_addr += chunk_size;
        }
        else { // read_cur_addr >= read_last_addr && read_frame_addr < read_cur_addr
            DPRINTF(LGVideoNW, "nw finish to read frame(%d)\n", pktPtr->frame_num);
            return true;
        }
    }

    return false;
}

bool
LGVideoNW::process_state()
{
    processing_cycles++;

    if(processing_timer == 0)
        SET_IP_TIMER(processing_timer,processing_latency);
    else if (processing_timer == 1) {
        if(state == IP_UPLOAD)
        {
            pktPtr->comm = IP_RESPONSE;
            IPResponseQueue.push_back(pktPtr);
            pktPtr = nullptr;
        }
        return true;
    }
    else
        processing_timer--;

    return false;
}

bool
LGVideoNW::write_state()
{
    write_cycles++;

    if(dma->isActive())
    {
    }
    else
    {
        if(write_frame_addr >= write_cur_addr) {
            DPRINTF(LGVideoNW, "nw start to write frame(%d)\n", pktPtr->frame_num);
            size_t chunk_size = write_last_addr > write_cur_addr + buffer_size
                ? buffer_size : write_last_addr - write_cur_addr;
            dma->flush();
            dma->startFlush(write_cur_addr, chunk_size);
            write_cur_addr += chunk_size;
        }
        else if (write_cur_addr < write_last_addr) {
            size_t chunk_size = write_last_addr > write_cur_addr + buffer_size
                ? buffer_size : write_last_addr - write_cur_addr;
            dma->flush();
            dma->startFlush(write_cur_addr, chunk_size);
            write_cur_addr += chunk_size;
        }
        else { // read_cur_addr >= read_last_addr && read_frame_addr < read_cur_addr
            DPRINTF(LGVideoNW, "nw finish to write frame(%d)\n", pktPtr->frame_num);
            pktPtr->comm = IP_RESPONSE;
            IPResponseQueue.push_back(pktPtr);
            pktPtr = nullptr;
            return true;
        }
    }

    return false;
}

    bool
LGVideoNW::sleep_state()
{
    sleep_cycles++;

    if(!IPRequestQueue.empty()) { // time to wakeup
        if(wakeup_timer == 0)
            wakeup_timer = wakeup_latency;
        else if (wakeup_timer == 1)
            return true;
        else
            wakeup_timer--;
    }
    return false;
}

void
LGVideoNW::update_stats()
{
}

void
LGVideoNW::regStats()
{
    MemObject::regStats();
    idle_cycles.name(name() + ".idle_cycles")
        .desc("Number of idle cycles")
        ;
    read_cycles.name(name() + ".read_cycles")
        .desc("Number of read cycles")
        ;
    processing_cycles.name(name() + ".processing_cycles")
        .desc("Number of processing cycles")
        ;
    write_cycles.name(name() + ".write_cycles")
        .desc("Number of write cycles")
        ;
    sleep_cycles.name(name() + ".sleep_cycles")
        .desc("Number of sleep cycles")
        ;
    active_cycles.name(name() + ".active_cycles")
        .desc("Number of active cycles")
        ;
    slack_latency.name(name() + ".slack_cycles")
        .desc("Number of slack cycles")
        .init(16) // number of buckets
        ;
}

void
LGVideoNW::resetStats()
{
    MemObject::resetStats();
}

void
LGVideoNW::printPeriodStats()
{
}

LGVideoNW*
LGVideoNWParams::create()
{
    return new LGVideoNW(this);
}
