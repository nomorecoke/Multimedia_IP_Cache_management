#ifndef __MULTIMEDIA_HH__
#define __MULTIMEDIA_HH__

#include <deque>
#include <string>

#include "sim/system.hh"
#include "sim/sim_exit.hh"
#include "mem/mem_object.hh"

#include "LG/lg_buffer_queue.hh"

#define FIXED_RANGE 2*1024*1024

enum IP_Type {
    VD_MODULE = 0,
    VE_MODULE,
    NW_DOWNLOAD,
    NW_UPLOAD,
    DC_MODULE,
    CAM_MODULE
};

enum IP_Command {
    IP_REQUEST = 0,
    IP_RESPONSE,
    IP_REQUEST_RETRY,
    IP_RESPONSE_RETRY
};

enum IP_State {
    IP_IDLE = 0,
    IP_SLEEP,
    IP_READ,
    IP_WRITE,
    IP_DECODE,       // For Video Deocder
    IP_ENCODE,       // For Video Encoder
    IP_DOWNLOAD,     // For Network Module
    IP_UPLOAD,       // For Network Module
    IP_IMAGING,      // For Camera Module
    IP_DISPLAY,      // For Display Controller
    IP_GOSLEEP,
    IP_WAKEUP
};
/*
std::string ips2n(IP_State s)
{
    switch(s) {
        case IP_IDLE:
            return "IP_IDLE";
        case IP_SLEEP:
            return "IP_IDLE";
        case IP_READ:
            return "IP_READ";
        case IP_WRITE:
            return "IP_WRITE";
        case IP_DECODE:
            return "IP_DECODE";
        case IP_ENCODE:
            return "IP_ENCODE";       // For Video Encoder
        case IP_DOWNLOAD:
            return "IP_DOWNLOAD";    // For Network Module
        case IP_UPLOAD:
            return "IP_UPLOAD";       // For Network Module
        case IP_IMAGING:
            return "IP_IMAGING";// For Camera Module
        case IP_DISPLAY:
            return "IP_DISPLAY";// For Display Controller
        case IP_GOSLEEP:
            return "IP_GOSLEEP";
        case IP_WAKEUP:
            return "IP_WAKEUP";
        default:
            panic("Unknown state");
            return NULL;
    };
}
*/

class IP_Packet {
    public:
        IP_Type type;
        IP_Command comm;
        Addr read_addr;
        Addr write_addr;
        
        int frame_num;
        int curSliceNum = 0;

        IP_Packet() {}
        virtual ~IP_Packet() {}
};

#define SET_CODER_REQUEST(_pktPtr, _t, _c, _r_a, _w_a, _f_n) { \
    _pktPtr->type = _t; \
    _pktPtr->comm = _c; \
    _pktPtr->read_addr = _r_a; \
    _pktPtr->write_addr = _w_a; \
    _pktPtr->frame_num = _f_n; }

#define SET_NW_REQUEST(_pktPtr, _t, _c, _r_a, _w_a, _f_n) { \
    _pktPtr->type = _t; \
    _pktPtr->comm = _c; \
    _pktPtr->read_addr = _r_a; \
    _pktPtr->write_addr = _w_a; \
    _pktPtr->frame_num = _f_n; }

#define SET_DC_REQUEST(_pktPtr, _t, _c, _r_a, _f_n) { \
    _pktPtr->type = _t; \
    _pktPtr->comm = _c; \
    _pktPtr->read_addr = _r_a; \
    _pktPtr->frame_num = _f_n; }

#define SET_CAM_REQUEST(_pktPtr, _t, _c, _w_a, _f_n) { \
    _pktPtr->type = _t; \
    _pktPtr->comm = _c; \
    _pktPtr->write_addr = _w_a; \
    _pktPtr->frame_num = _f_n; }

#define SLICE_REQUEST(_pktPtr, _sm, _dss, _ess, _csm) { \
    _pktPtr->sliceMode = _sm; \
    _pktPtr->decodedSliceSize = _dss; \
    _pktPtr->encodedSliceSize = _ess; \
    _pktPtr->curSliceNum = _csm; \
}

#define RESET_IP_TIMER(t) (t = 0)
#define SET_IP_TIMER(t, delay) (t = delay)

#endif
