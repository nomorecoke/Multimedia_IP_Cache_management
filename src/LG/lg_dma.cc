#include "LG/lg_dma.hh"
#include "base/intmath.hh"

#include <iostream>
#include <string.h>
#include <fstream>

#define GOT_HERE std::cout << "At " __FILE__":" << __LINE__ << std::endl

LGDma::LGDma(DmaPort& _port, unsigned _max_req, BufferQueue* _bufferQueue, std::string _name)
    : port(_port), max_req(_max_req), bufferQueue(_bufferQueue), name(_name)
{
    freeRequests.clear();
    pendingRequests.clear();

    for(int ii = 0; ii < max_req; ii++) {
        DmaEvent* event = new DmaEvent(this, ii, nullptr);
        freeRequests.push_back(event);
    }

    freeReqCnt = max_req;
    pendingReqCnt = 0;

    std::cout << "Number of Dma requests: " << freeRequests.size() << std::endl;
};

void
LGDma::printBufferTrace() {
#if BUFFER_TRACE
    GOT_HERE;
    std::string filename = name + ".csv";
    std::ofstream outFile(filename);
    for (auto &it : bufferProfile) {
        outFile << it.first;
        for (auto inner_it : it.second) 
            outFile << "," << inner_it;
        outFile << std::endl;
    }
    outFile.close();
#endif
}

LGDma::DmaEvent*
LGDma::dequeueDmaRequest() {
    if (!freeRequests.empty()) {
        DmaEvent* event = freeRequests.front();
        freeRequests.pop_front();
        freeReqCnt--;
        return event;
    }
    return nullptr;
}

void
LGDma::enqueueDmaRequest(DmaEvent* event)
{
    pendingReqCnt++;
    pendingRequests.push_back(event);
}

LGDma::DmaEvent* 
LGDma::acquireDmaRequest()
{
    if (!pendingRequests.empty()) {
        DmaEvent* event = pendingRequests.front();
        pendingRequests.pop_front();
        pendingReqCnt--;
        return event;
    }
    return nullptr;
}

void
LGDma::releaseDmaRequest(DmaEvent* event)
{
    freeReqCnt++;
    freeRequests.push_back(event);
}

LGDma::DmaEvent* 
LGDma::findCompleteDmaEvent(int id)
{
    int pendingQueueSize = pendingReqCnt;
    for (int ii = 0; ii < pendingQueueSize; ii++) {
        DmaEvent* event = acquireDmaRequest();
        if (event->id == id) {
            return event;
        }
        else
            enqueueDmaRequest(event);
    }
    return nullptr;
}

bool
LGDma::issueDmaRequest(Packet::Command cmd, Addr addr, int size, VBuffer* _buffer)
{
    DmaEvent* event = nullptr;
    if ((event = dequeueDmaRequest())) {
        event->buffer = _buffer;

        if (cmd == Packet::Command::ReadReq)
            event->buffer->load_latency = curTick();
        else
            event->buffer->store_latency = curTick();

        memset(event->data, 0, _buffer->buffer_size);
        port.dmaAction(cmd, addr, size, event, event->data, 0, flags);
        enqueueDmaRequest(event);
        return true;
    }
    
    return false;
}

bool
LGDma::issueDmaRequest(Packet::Command cmd, Addr addr, int size, VBuffer* _buffer, bool isCacheable)
{
     
    Request::Flags tempFlags = 0x0;
    if (!isCacheable) {
        tempFlags = Request::UNCACHEABLE;
    }

    DmaEvent* event = nullptr;
    if ((event = dequeueDmaRequest())) {
        event->buffer = _buffer;

        if (cmd == Packet::Command::ReadReq)
            event->buffer->load_latency = curTick();
        else
            event->buffer->store_latency = curTick();

        memset(event->data, 0, _buffer->buffer_size);
        port.dmaAction(cmd, addr, size, event, event->data, 0, tempFlags);
        enqueueDmaRequest(event);
        return true;
    }
    
    return false;
}

void
LGDma::completeDmaRequest(int id)
{
    DmaEvent* event = nullptr;
    if ((event = findCompleteDmaEvent(id)))
    {
        if (event->buffer->state == LOAD)
        {
            event->buffer->load_latency = curTick() - event->buffer->load_latency; 
            bufferQueue->enqueueBuffer(event->buffer);
#if BUFFER_TRACE
            bufferProfile[event->buffer->start_addr].push_back(event->buffer->load_latency);
#endif
            event->buffer = nullptr;
        }
        else if (event->buffer->state == STORE)
        {
            event->buffer->store_latency = curTick() - event->buffer->store_latency; 
            bufferQueue->releaseBuffer(event->buffer);
#if BUFFER_TRACE
            bufferProfile[event->buffer->start_addr].push_back(event->buffer->store_latency);
#endif
            event->buffer = nullptr;
        }
        else
        {
            panic("Unknown complete buffer state");
        }

        releaseDmaRequest(event);
    }
    else 
    {
        std::cout << id << "," << pendingReqCnt << "," << freeReqCnt << std::endl;
        for (int ii = 0; ii < pendingReqCnt; ii++) {
            std::cout << pendingRequests[ii]->id << std::endl;
        }
        panic("There is no complete event in pending queue");
    }
}

LGDma::DmaEvent::DmaEvent(LGDma* _dev, int _id, VBuffer* _buffer) 
    : dev(_dev), buffer(_buffer), id(_id)
{
}

void
LGDma::DmaEvent::process()
{
    dev->completeDmaRequest(id);
}
