#ifndef __LG_VIDEO_NW_HH__
#define __LG_VIDEO_NW_HH__

#include <deque>
#include <stdint.h>
#include <string>

#include "LG/multimedia.hh"
#include "dev/dma_device.hh"
#include "mem/mem_object.hh"

#include "debug/LGVideoNW.hh"
#include "params/LGVideoNW.hh"

#include "LG/ip_packet_queue.hh"

class LGVideoNW : public MemObject
{
    public:
        LGVideoNW(const LGVideoNWParams *params);
        virtual ~LGVideoNW();

        void init();

        void tick();

        void transition();

        bool idle_state();
        bool sleep_state();
        bool process_state();
        bool read_state();
        bool write_state();

        void update_stats();
        void resetStats();
        void regStats();
        void printPeriodStats();

        EventWrapper<LGVideoNW, &LGVideoNW::tick> tickEvent;

        BaseMasterPort& getMasterPort(const std::string& if_name,
                PortID idx = InvalidPortID);

        std::deque<IP_Packet*> IPRequestQueue;
        IP_Packet* pktPtr;

        Stats::Scalar idle_cycles;
        Stats::Scalar read_cycles;
        Stats::Scalar processing_cycles;
        Stats::Scalar write_cycles;
        Stats::Scalar sleep_cycles;
        Stats::Scalar active_cycles;
		Stats::Histogram slack_latency;

        long slack_cycles;

        IP_State state;

    protected:
        size_t buffer_size;
        size_t full_frame_size;
        size_t encoded_frame_size;
        size_t cache_line_size;

        Tick setup_delay;


        Addr read_frame_addr;
        Addr write_frame_addr;
        Addr read_cur_addr;
        Addr write_cur_addr;
        Addr read_last_addr;
        Addr write_last_addr;

        long processing_latency;
        long sleep_latency;
        long wakeup_latency;

        long processing_timer;
        long sleep_timer;
        long wakeup_timer;

        DmaPort port;
        DmaFifo* dma;

        IPPacketQueue& IPResponseQueue;
};

#endif
