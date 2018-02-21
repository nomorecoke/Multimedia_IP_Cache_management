/*
 * 2017 - 11 - 19 YW
 * dynamic insertion policy with segment size
 * count up miss count and find the best
 * 
 */

#ifndef __MEM_CACHE_TAGS_PDP_HH__
#define __MEM_CACHE_TAGS_PDP_HH__

#include "mem/cache/tags/base_set_assoc.hh"
#include "utils.hh"
#include "params/PDPS.hh"
#include <vector>
#include <algorithm>
#define USE_INSERTION_POLICY 5 // 5 size of segment (1,4,8,12,16) for 16 way
#define USE_SDMsize 32 // 32 sets per SDM 

#define PDP_TYPE_NUM 5
#define PDP_Leader_set_num 8 // 8 set for one policy  8 * 5 = 40 leader set

#define MAX_RD 256
typedef enum
{
    PDP_FOLLOWERS = 0,
    PDP_SIZE_1 = 1,
    PDP_SIZE_4 = 2,
    PDP_SIZE_8 = 3,
    PDP_SIZE_12 = 4,
    PDP_SIZE_16 = 5
} PDP_Type_t; // 1, 0.25 * assoc, 0.5 * assoc, 0.75*assoc, assoc

struct RD_sampler
{
    int set;
    int tag;
    int cpu;
    int last_access;

    RD_sampler(){}
    RD_sampler(int _set,int _tag, int _cpu, int _last_access){
        set = _set; tag=_tag;cpu = _cpu; last_access = _last_access;
    }

};

//unsigned long PDP_max_rand;
class PDPS : public BaseSetAssoc
{
    public:
        typedef PDPSParams Params;
        PDPS(const Params *p);

        ~PDPS(){}
        
        CacheBlk* accessBlock(Addr addr, bool is_secure, Cycles &lat,MasterID master_id);
        CacheBlk* findVictim(Addr addr, MasterID master_id);
        void insertBlock(PacketPtr pkt, BlkType *blk);
        void invalidate(CacheBlk *blk);
        void RandomlyChooseLeader();
        int getSegmentSize(uint32_t set ,int isUpdate); // return segment size 
        void RecordHit(uint32_t set);
        void RecordMiss(uint32_t set);
        void SetMonitor(void);

        /* RD Sampler for PDP*/
        void RDSampler(Addr addr,MasterID master_Id);
        void PD_Compute_Logic(void);
        void PDP_Controller(int set);
        void update_policy();
        EventWrapper<PDPS,&PDPS::update_policy> updatePolicy;

        /* variable*/
        int* PDP_monitor;
        int  PDP_MissCounter[PDP_TYPE_NUM];
        unsigned long PDP_numset;
        int PDP_current_seg_size = 0;
        int segment_size_array[5] = {2,4,8,12,16};
 
        /* PDP variable */
        vector<RD_sampler> sampler;
        int* set_counter;
        int** rd_counter;

        int* opt_PD;
        int RD_init = 0;
        int num_of_CPU;


};

#endif // __MEM_CACHE_TAGS_PDP_HH__
