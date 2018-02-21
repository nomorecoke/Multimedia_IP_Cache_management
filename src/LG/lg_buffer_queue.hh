#ifndef __LG_BUFFER_QUEUE_HH__
#define __LG_BUFFER_QUEUE_HH__

#include "mem/mem_object.hh"
#include <deque>

#define MAX_BUFFER_NUM 2048

enum BufferState {
    EMPTY,
    LOAD,
    PROCESS,
    STORE
};

class VBuffer {
    public:
        BufferState state;
        Addr start_addr;
        size_t buffer_size;

        Tick load_latency;
        Tick process_latency;
        Tick store_latency;

    VBuffer() {
        state = EMPTY;
        start_addr = 0x0;
        buffer_size = 0x0;

        load_latency = 0;
        process_latency = 0;
        store_latency = 0;
    }
};

class BufferQueue {
    public:

        BufferQueue(size_t _buffer_entry_size,
                    size_t _buffer_queue_size);

        ~BufferQueue();
        
        VBuffer* getFreeBuffer();
        VBuffer* getUsedBuffer();
        void enqueueBuffer(VBuffer* usedBuffer);
        void releaseBuffer(VBuffer* usedBuffer);
    
        size_t buffer_num;
        size_t buffer_entry_size;
        size_t buffer_queue_size;

        int freeBuffer_num;
        int usedBuffer_num;

        std::deque<VBuffer*> freeQueue;
        std::deque<VBuffer*> usedQueue;

        VBuffer* buffer_entry[MAX_BUFFER_NUM];
};

#endif
