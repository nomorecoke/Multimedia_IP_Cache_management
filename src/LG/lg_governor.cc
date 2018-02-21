#include "LG/lg_governor.hh"
#include <string>
#include <iostream>
#include <fstream>

#define FRAME_TO_DISPLAY 30

LGGovernor::LGGovernor(const LGGovernorParams *params)
    : MemObject(params),
      tickEvent(*this),
      frameEvent(*this),
      getPeriod(*this),
      vd(*params->video_decoder),
      cam(*params->video_camera),
      ve(*params->video_encoder),
      dc(*params->video_display),
      nw(*params->video_nw),
      IPResponseQueue(*params->response_queue),
      fps(params->frame_per_seconds),
      setup_delay(params->setup_delay),
      sliceMode(params->sliceMode),
      full_frame_size_in(params->full_frame_size_in),
      full_frame_size_out(params->full_frame_size_out),
      encoded_frame_size_in(params->encoded_frame_size_in),
      encoded_frame_size_out(params->encoded_frame_size_out),
      totalSliceCount_in(params->totalSliceCount_in),
      totalSliceCount_out(params->totalSliceCount_out)
{
    // TODO: make flow type option
    if(params->flow_type == "YouTube")
        flow_type = YouTube;
    else if(params->flow_type == "Skype")
        flow_type = Skype;
    else {
        std::cout << "Unknown flow type" << std::endl;
        flow_type = YouTube;
    }

    frame_period = (Tick)(1000000000000/fps);

    frame_count = 0;

    read_frame_done = 0;
    write_frame_done = 0;
    init(params);

    nsec_period = (Tick)(1000);
    microsec_period = 1000 * nsec_period;
    msec_period = 1000 * microsec_period;

    // Slice-mode initialization
    for (int ii = 0; ii < 1024; ii++) {
        curSliceNum_in[ii] = 0;
        curSliceNum_out[ii] = 0;
    }
}

LGGovernor::~LGGovernor()
{
}

void
LGGovernor::init(const LGGovernorParams *params) {
/*
 */


    schedule(tickEvent, 500000000000);
  
    schedule(frameEvent, 500000000000);

    schedule(getPeriod, 500000000000);
}

void
LGGovernor::tick() {
    handle_response();
    if (flow_type == YouTube) {
        if(read_frame_done >= FRAME_TO_DISPLAY) {
            printStat();
            exitSimLoop("Done", curTick());
        }
    }
    if (flow_type == Skype) {
        if(read_frame_done >= FRAME_TO_DISPLAY 
                && write_frame_done >= FRAME_TO_DISPLAY) {
            printStat();
            exitSimLoop("Done", curTick());
        }
    }
    schedule(tickEvent, clockEdge(Cycles(1)));
}

void
LGGovernor::frame_signal() {
    if (frame_count >= FRAME_TO_DISPLAY)
        return ;

	frame_count++;

    if (flow_type == YouTube) {
        IP_Packet *pktPtr = new IP_Packet;
        SET_NW_REQUEST(pktPtr, NW_DOWNLOAD, IP_REQUEST, 0x0, VD_ADDR_START, frame_count);

        nw.IPRequestQueue.push_back(pktPtr);
        displayLatency.insert(std::map<int, Tick>::value_type(frame_count, curTick()));
    }
    else if (flow_type == Skype){
        IP_Packet *pktPtr = new IP_Packet;
        SET_NW_REQUEST(pktPtr, NW_DOWNLOAD, IP_REQUEST, 0x0, VD_ADDR_START, frame_count);
        nw.IPRequestQueue.push_back(pktPtr);
        displayLatency.insert(std::map<int, Tick>::value_type(frame_count, curTick()));
        
        IP_Packet *pktPtr2 = new IP_Packet;
        SET_CAM_REQUEST(pktPtr2, CAM_MODULE, IP_REQUEST, VE_ADDR_START, frame_count);
        cam.IPRequestQueue.push_back(pktPtr2);
        recordLatency.insert(std::map<int, Tick>::value_type(frame_count, curTick()));
    }
    else {
        panic("Unknown flow type\n");
    }
    std::cout << "frame Signaling @ " << curTick() << std::endl;
    schedule(frameEvent, curTick() + frame_period);
}

void 
LGGovernor::handle_response() {
    if (IPResponseQueue.empty())
        return ;
    
    IP_Packet *pktPtr = IPResponseQueue.front(); IPResponseQueue.pop_front();
    if (!sliceMode) {
        switch(pktPtr->type) {
            case VD_MODULE:         // Send request to DC for displaying
                SET_DC_REQUEST(pktPtr, DC_MODULE, IP_REQUEST, DC_ADDR_START, pktPtr->frame_num);
                dc.IPRequestQueue.push_back(pktPtr);
                break;
            case VE_MODULE:         // Send request to NW for uploading
                SET_NW_REQUEST(pktPtr, NW_UPLOAD, IP_REQUEST, NW_ADDR_START, 0x0, pktPtr->frame_num);
                nw.IPRequestQueue.push_back(pktPtr);
                break;
            case NW_DOWNLOAD:       // Send request to VD for decoding
                SET_CODER_REQUEST(pktPtr, VD_MODULE, IP_REQUEST, VD_ADDR_START, DC_ADDR_START, pktPtr->frame_num);
                vd.IPRequestQueue.push_back(pktPtr);
                break;
            case NW_UPLOAD:         // Flow is over;
                // TODO: uploading frmae is over
                write_frame_done++;
                std::cout << "video frame record count:" << write_frame_done << std::endl;
                delete pktPtr;
                break;
            case DC_MODULE:         // Flow is over;
                // TODO: display is over
                read_frame_done++;
                std::cout << "video frame playback count:" << read_frame_done << std::endl;
                displayLatency.find(pktPtr->frame_num)->second = 
                    curTick() - displayLatency.find(pktPtr->frame_num)->second; 
                delete pktPtr;
                break;
            case CAM_MODULE:        // Send request to VE for encoding
                SET_CODER_REQUEST(pktPtr, VE_MODULE, IP_REQUEST, VE_ADDR_START, NW_ADDR_START, pktPtr->frame_num);
                ve.IPRequestQueue.push_back(pktPtr);
                recordLatency.find(pktPtr->frame_num)->second 
                    = curTick() - recordLatency.find(pktPtr->frame_num)->second; 
                break;
            default:
                panic("Unknown IP Response\n");
                break;
        }
    }
    else {
        size_t readAddr, writeAddr;
        switch(pktPtr->type) {
            case VD_MODULE:         // Send request to DC for displaying
                readAddr = DC_ADDR_START + full_frame_size_in * pktPtr->curSliceNum;
                SET_DC_REQUEST(pktPtr, DC_MODULE, IP_REQUEST, readAddr, pktPtr->frame_num);
                dc.IPRequestQueue.push_back(pktPtr);
                break;
            case VE_MODULE:         // Send request to NW for uploading
                SET_NW_REQUEST(pktPtr, NW_UPLOAD, IP_REQUEST, NW_ADDR_START, 0x0, pktPtr->frame_num);
                nw.IPRequestQueue.push_back(pktPtr);
                break;
            case NW_DOWNLOAD:       // Send request to VD for decoding && sub-frame start!
                pktPtr->curSliceNum = 1;
                readAddr = VD_ADDR_START + encoded_frame_size_in * pktPtr->curSliceNum;
                writeAddr = DC_ADDR_START + full_frame_size_in * pktPtr->curSliceNum;;
                SET_CODER_REQUEST(pktPtr, VD_MODULE, IP_REQUEST, readAddr, writeAddr, pktPtr->frame_num);
                vd.IPRequestQueue.push_back(pktPtr);
                break;
            case NW_UPLOAD:         // Flow is over;
                // TODO: uploading frmae is over
                write_frame_done++;
                std::cout << "video frame record count:" << write_frame_done << std::endl;
                delete pktPtr;
                break;
            case DC_MODULE:         // Flow is over;
                // TODO: display is over
                if (pktPtr->curSliceNum < totalSliceCount_in) {
                    pktPtr->curSliceNum++;
                    readAddr = VD_ADDR_START + encoded_frame_size_in * pktPtr->curSliceNum;
                    writeAddr = DC_ADDR_START + full_frame_size_in * pktPtr->curSliceNum;;
                    SET_CODER_REQUEST(pktPtr, VD_MODULE, IP_REQUEST, readAddr, writeAddr, pktPtr->frame_num);
                    vd.IPRequestQueue.push_back(pktPtr);
                }
                else {
                    read_frame_done++;
                    std::cout << "video frame playback count:" << read_frame_done << std::endl;
                    displayLatency.find(pktPtr->frame_num)->second = 
                        curTick() - displayLatency.find(pktPtr->frame_num)->second; 
                    delete pktPtr;
                }
                break;
            case CAM_MODULE:        // Send request to VE for encoding
                SET_CODER_REQUEST(pktPtr, VE_MODULE, IP_REQUEST, VE_ADDR_START, NW_ADDR_START, pktPtr->frame_num);
                ve.IPRequestQueue.push_back(pktPtr);
                recordLatency.find(pktPtr->frame_num)->second 
                    = curTick() - recordLatency.find(pktPtr->frame_num)->second; 
                break;
            default:
                panic("Unknown IP Response\n");
                break;
        }
    }
}

void 
LGGovernor::printStat()
{
// Buffer Trace (frame address - latency)
    vd.read_dma->printBufferTrace();
    vd.write_dma->printBufferTrace();
    ve.read_dma->printBufferTrace();
    ve.write_dma->printBufferTrace();
    cam.write_dma->printBufferTrace();
    dc.read_dma->printBufferTrace();

// Buffer Usage
#if BUFFER_STAT
    std::string buffer_filename = "buffer_usage.csv";
    std::string packet_filename = "packet_usage.csv";

    std::ofstream bufferOut(buffer_filename);
    std::ofstream packetOut(packet_filename);
   /* 
    for(int ii = 0; ii < 6; ii++) {
        bufferOut << "IP" << ii;
        for(auto it : BufferUsage[ii])
            bufferOut << "," << it;
        bufferOut << std::endl;
    }
    */
    for(int ii = 0; ii < 6; ii++) {
        packetOut << "IP" << ii;
        for(auto it : dmaPacketUsage[ii])
            packetOut << "," << it;
        packetOut << std::endl;
    }

    bufferOut.close();
    packetOut.close();
    
#endif
    std::cout << "display latency";
    for (auto it : displayLatency) {
        std::cout << "," << it.second;
    }
    std::cout << std::endl;

    std::cout << "record latency";
    for (auto it : recordLatency) {
        std::cout << "," << it.second;
    }
    std::cout << std::endl;

}

void
LGGovernor::getPeriodStat()
{
   // Period start

   // Power stat

   // Energy stat

#if BUFFER_STAT
   // Buffer stat --> bufferQueue buffer usage, dma packet usage
    BufferUsage[0].push_back(vd.inputBufferQueue->buffer_num - vd.inputBufferQueue->freeBuffer_num);
    BufferUsage[1].push_back(vd.outputBufferQueue->buffer_num - vd.outputBufferQueue->freeBuffer_num);
    BufferUsage[2].push_back(ve.inputBufferQueue->buffer_num - ve.inputBufferQueue->freeBuffer_num);
    BufferUsage[3].push_back(ve.outputBufferQueue->buffer_num - ve.outputBufferQueue->freeBuffer_num);
    BufferUsage[4].push_back(dc.inputBufferQueue->buffer_num - dc.inputBufferQueue->freeBuffer_num);
    BufferUsage[6].push_back(cam.outputBufferQueue->buffer_num - cam.outputBufferQueue->freeBuffer_num);

    dmaPacketUsage[0].push_back(vd.read_dma->pendingRequests.size());
    dmaPacketUsage[1].push_back(vd.write_dma->pendingRequests.size());
    dmaPacketUsage[2].push_back(ve.read_dma->pendingRequests.size());
    dmaPacketUsage[3].push_back(ve.write_dma->pendingRequests.size());
    dmaPacketUsage[4].push_back(dc.read_dma->pendingRequests.size());
    dmaPacketUsage[5].push_back(cam.write_dma->pendingRequests.size());
#endif
//    schedule(getPeriod, curTick() + (Tick)(1000000000));   // Per 1000 microsec
//    schedule(getPeriod, curTick() + (Tick)(100000000));    // Per 100 microsec
//    schedule(getPeriod, curTick() + (Tick)(100000000));    // Per 10 microsec
    schedule(getPeriod, curTick() + (Tick)(1000000));      // Per 1 microsec
}


LGGovernor*
LGGovernorParams::create()
{
    return new LGGovernor(this);
}
