#ifndef __IP_PACKET_QUEUE__
#define __IP_PACKET_QUEUE__

#include <deque>

#include "LG/multimedia.hh"
#include "params/IPPacketQueue.hh"
#include "sim/sim_object.hh"

#include <list>

class IPPacketQueue : public SimObject
{
    private:
        std::deque<IP_Packet*> packetQ;

    public:
        IPPacketQueue(const IPPacketQueueParams *params);
        virtual ~IPPacketQueue();
        
        bool empty() { return packetQ.empty(); }
        void push_back(IP_Packet* pktPtr) { packetQ.push_back(pktPtr); }
        void pop_front() { packetQ.pop_front(); }
        IP_Packet* front() { return packetQ.front(); }

        bool decoderBusy = false;
        bool encoderBusy = false;
        bool camBusy = false;
        bool dcBusy = false;
        bool nwBusy = false;

        /*
        // For buffer-state aware caching
        class caching_list_entry {
            public:
                caching_list_entry();
                ~caching_list_entry();

                int priority;
                int id;
                Addr start_addr;
                size_t length;
        };
        */

};

#endif
