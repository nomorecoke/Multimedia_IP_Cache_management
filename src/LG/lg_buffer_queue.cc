#include "LG/lg_buffer_queue.hh"
#include "base/intmath.hh"

#include <iostream>

BufferQueue::BufferQueue(size_t _buffer_entry_size,
                         size_t _buffer_queue_size)
{
    buffer_entry_size = _buffer_entry_size;
    buffer_queue_size = _buffer_queue_size;
    buffer_num = divCeil(buffer_queue_size, buffer_entry_size);

    freeQueue.clear();
    usedQueue.clear();

    freeBuffer_num = 0;
    usedBuffer_num = 0;

    for (int ii = 0; ii < buffer_num; ii++) {
        buffer_entry[ii] = new VBuffer();
        freeQueue.push_back(buffer_entry[ii]);
        freeBuffer_num++;
    }

    std::cout << "Buffer Queue:" << buffer_entry_size << ","
                                 << buffer_num << ","
                                 << buffer_queue_size << std::endl;
}

BufferQueue::~BufferQueue()
{
    for (int ii = 0; ii < buffer_num; ii++)
        delete buffer_entry[ii];
}

VBuffer*
BufferQueue::getFreeBuffer() {
    if (!freeQueue.empty()) {
        VBuffer* freeBuffer = freeQueue.front();
        freeQueue.pop_front(); freeBuffer_num--;
        return freeBuffer;
    }
    return nullptr;
}


VBuffer*
BufferQueue::getUsedBuffer() {
    if (!usedQueue.empty()) {
        VBuffer* usedBuffer = usedQueue.front();
        usedQueue.pop_front(); usedBuffer_num--;
        return usedBuffer;
    }
    return nullptr;
}

void 
BufferQueue::releaseBuffer(VBuffer* usedBuffer) {
    freeBuffer_num++;
    freeQueue.push_back(usedBuffer);
}

void 
BufferQueue::enqueueBuffer(VBuffer* usedBuffer) {
    usedBuffer_num++;
    usedQueue.push_back(usedBuffer);
}
