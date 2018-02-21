#ifndef __LG_VIDEO_ENCODER_HH__
#define __LG_VIDEO_ENCODER_HH__

#include <deque>
#include <stdint.h>
#include <string>

#include "LG/multimedia.hh"
#include "dev/dma_device.hh"
#include "mem/mem_object.hh"
#include "LG/ip_packet_queue.hh"
#include "LG/lg_buffer_queue.hh"
#include "LG/lg_dma.hh"

#include "debug/LGVideoEncoder.hh"
#include "params/LGVideoEncoder.hh"

class LGVideoEncoder : public MemObject
{
    public:
        LGVideoEncoder(const LGVideoEncoderParams *params);
        virtual ~LGVideoEncoder();

        BaseMasterPort& getMasterPort(const std::string& if_name,
                PortID idx = InvalidPortID);

        std::deque<IP_Packet*> IPRequestQueue;
        IPPacketQueue& IPResponseQueue;
        IP_Packet* pktPtr;

        VBuffer* readBufferPtr;
        VBuffer* processBufferPtr;
        VBuffer* writeBufferPtr;

        void Initialize();
        void tick();
        bool frameLoad();
        bool frameStore();
        bool frameProcess();

        void regStats();
        void resetStats();
        
        EventWrapper<LGVideoEncoder, &LGVideoEncoder::tick> tickEvent;
    
        Stats::Scalar idle_cycles;
        Stats::Scalar active_cycles;
        Stats::Scalar true_processing_cycles;
        Stats::Scalar data_stall_cycles;
        Stats::Scalar read_stall_cycles;
        Stats::Scalar write_stall_cycles;

        Stats::Scalar input_buffer_full;
        Stats::Scalar output_buffer_empty;

    protected:

        Addr read_start_addr;
        Addr read_cur_addr;
        Addr read_last_addr;

        Addr write_start_addr;
        Addr write_cur_addr;
        Addr write_last_addr;

        Addr process_start_addr;
        Addr process_cur_addr;
        Addr process_last_addr;
        Addr process_mark_addr;

        Cycles processing_cycles;

        size_t full_frame_size;
        size_t encoded_frame_size;
        size_t buffer_size;
        size_t input_buffer_size;
        size_t output_buffer_size;
        size_t coding_ratio;
		size_t processedBufferCnt;

    public:
        bool isBusy;

        BufferQueue* inputBufferQueue;
        BufferQueue* outputBufferQueue;

        DmaPort read_port;
        DmaPort write_port;

        LGDma* read_dma;
        LGDma* write_dma;

//        DmaFifo* read_dma;
//        DmaFifo* write_dma;
};

#endif
