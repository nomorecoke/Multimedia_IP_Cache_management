#ifndef __LG_DMA_HH__
#define __LG_DMA_HH__

#include "LG/multimedia.hh"
#include "dev/dma_device.hh"

#include <string>
#include <deque>
#include <map>

#define BUFFER_TRACE 0

class LGDma
{
    public:
        LGDma(DmaPort& _port, unsigned _max_req, BufferQueue* _bufferQueue, std::string _name);

    public:
        class DmaEvent : public Event
        {
            public:
                LGDma* dev;
                VBuffer* buffer;
                int id;
                uint8_t data[1024];

                DmaEvent(LGDma* _dev, int _id, VBuffer* _buffer);

                void process();
        };

    public:
        DmaPort& port;   // Dma Port
        const unsigned max_req; // Maximum requests that can be queued.
        Request::Flags flags = Request::UNCACHEABLE;
        
        std::deque<DmaEvent*> freeRequests;
        std::deque<DmaEvent*> pendingRequests;

        size_t freeReqCnt;
        size_t pendingReqCnt;

        BufferQueue* bufferQueue;

        std::string name;
#if BUFFER_TRACE
        typedef std::deque<Tick> records;
        std::map<Addr, records> bufferProfile;
#endif

    public:
        DmaEvent* dequeueDmaRequest();
        void enqueueDmaRequest(DmaEvent* event);
        DmaEvent* acquireDmaRequest();
        void releaseDmaRequest(DmaEvent* event);

        DmaEvent* findCompleteDmaEvent(int id);

        bool issueDmaRequest(Packet::Command cmd, Addr addr, int size, VBuffer* _buffer);
        bool issueDmaRequest(Packet::Command cmd, Addr addr, int size, VBuffer* _buffer, bool isCacheable);
        void completeDmaRequest(int id);

        void printBufferTrace();
};

#endif
