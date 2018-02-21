#include "LG/ip_packet_queue.hh"

IPPacketQueue::IPPacketQueue(const IPPacketQueueParams *params)
    : SimObject(params)
{
    packetQ.clear();
}

IPPacketQueue::~IPPacketQueue()
{
}

IPPacketQueue*
IPPacketQueueParams::create()
{
    return new IPPacketQueue(this);
}
