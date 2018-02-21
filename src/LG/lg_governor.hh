#ifndef __LG_GOVERNOR_HH__
#define __LG_GOVERNOR_HH__

#include <deque>
#include <string>
#include <stdint.h>

#include "LG/multimedia.hh"
#include "dev/dma_device.hh"
#include "mem/mem_object.hh"

#include "debug/LGGovernor.hh"
#include "params/LGGovernor.hh"

#include "LG/lg_video_decoder.hh"
#include "LG/lg_video_encoder.hh"
#include "LG/lg_video_cam.hh"
#include "LG/lg_video_dc.hh"
#include "LG/lg_video_nw.hh"
#include "LG/ip_packet_queue.hh"
#include <deque>
#include <map>

#define PERIODIC_STAT 1
#define BUFFER_STAT 0

/* Add to the Scripts

   # IP_address_base = 0x70000000
   # IP_range_size = 0x10000000
   self.mem_ranges = [AddrRange(mem_size),
                      AddrRange(IP_address_base, size = IP_range_size), # For IP
                      ]
*/

#define DC_ADDR_START         0x72000000
#define CAM_ADDR_START        0x74000000
#define VD_ADDR_START         0x76000000
#define VE_ADDR_START         0x78000000
#define NW_ADDR_START         0x7A000000


/**
  *
  */

/**
  *
  * @return
  */


enum APP_type {
    YouTube = 0,
    Skype
};

class LGGovernor : public MemObject
{
    public:   
        LGGovernor(const LGGovernorParams *params);
        virtual ~LGGovernor();

        void init(const LGGovernorParams *params);

        void tick();
        void frame_signal();
        void handle_response();
        void getPeriodStat();
        void printStat();

        EventWrapper<LGGovernor, &LGGovernor::tick> tickEvent;
        EventWrapper<LGGovernor, &LGGovernor::frame_signal> frameEvent;
        EventWrapper<LGGovernor, &LGGovernor::getPeriodStat> getPeriod;
        

        LGVideoDecoder& vd;
        LGVideoCamera& cam;
        LGVideoEncoder& ve;
        LGVideoDisplay& dc;
        LGVideoNW& nw;

        IPPacketQueue& IPResponseQueue;

#if BUFFER_STAT
        std::deque<int> BufferUsage[6];
        std::deque<int> dmaPacketUsage[6];
#endif
        std::map<int, Tick> displayLatency;
        std::map<int, Tick> recordLatency;

    protected:
        int fps;
        Tick frame_period;
        int setup_delay;
        int frame_count;
        int read_frame_done;
        int write_frame_done;
        APP_type flow_type;

        Tick nsec_period;
        Tick microsec_period;
        Tick msec_period;

        bool sliceMode = false;
        size_t full_frame_size_in = 0;
        size_t full_frame_size_out = 0;
        size_t encoded_frame_size_in = 0;
        size_t encoded_frame_size_out = 0;
        int totalSliceCount_in = 1;
        int totalSliceCount_out = 1;
        int curSliceNum_in[1024];
        int curSliceNum_out[1024];
};

#endif
