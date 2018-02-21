/*
 * 2017 - 11 - 19 YW
 * dynamic insertion policy with segment size
 * count up miss count and find the best
 * 
 */

#ifndef __MEM_CACHE_TAGS_DCS_HH__
#define __MEM_CACHE_TAGS_DCS_HH__

#include "mem/cache/tags/base_set_assoc.hh"
#include "utils.hh"
#include "params/DCS.hh"

#define USE_INSERTION_POLICY 5 // 5 size of segment (1,4,8,12,16) for 16 way
#define USE_SDMsize 32 // 32 sets per SDM 

#define DCS_TYPE_NUM 5
#define DCS_Leader_set_num 32 // 8 set for one policy  8 * 5 = 40 leader set

#define elapsedRRPV 15

typedef enum
{
    DCS_FOLLOWERS = 0,
    DCS_SIZE_1 = 1,
    DCS_SIZE_4 = 2,
    DCS_SIZE_8 = 3,
    DCS_SIZE_12 = 4,
    DCS_SIZE_16 = 5
} DCS_Type_t; // 1, 0.25 * assoc, 0.5 * assoc, 0.75*assoc, assoc

//unsigned long dcs_max_rand;
class DCS : public BaseSetAssoc
{
    public:
        typedef DCSParams Params;
        DCS(const Params *p);

        ~DCS(){}
        
        // search the tag directory
        CacheBlk* accessBlock(Addr addr, bool is_secure, Cycles &lat,MasterID master_Id);

        // find a victim cache block to evict
        CacheBlk* findVictim(Addr addr, MasterID master_id);

        // insert a block into cache
        void insertBlock(PacketPtr pkt, BlkType *blk);

        // invalidate a cache block in cache
        void invalidate(CacheBlk *blk);

        // initialize
        void RandomlyChooseLeader();

        // Periodically detect optimal segment size
        int getSegmentSize(uint32_t set ,int isUpdate); // return segment size 

        // check available way for IP //
        int isAvailable(Addr addr, bool is_secure, MasterID master_Id);
        /* For monitor */
        void RecordHit(uint32_t set);
        void RecordMiss(uint32_t set);
        void SetMonitor(void);
        void update_policy(void);
        /* Event wrapper */
        EventWrapper<DCS, &DCS::update_policy> updatePolicy;

        /* variable*/
        int* DCS_monitor;
        int  DCS_MissCounter[DCS_TYPE_NUM];
        unsigned long DCS_numset;
        int DCS_current_seg_size = 0;
        int segment_count =0;
        int segment_sum =0;

        int segment_size_array[5] = {2,4,6,8,10};

};

#endif // __MEM_CACHE_TAGS_DCS_HH__
