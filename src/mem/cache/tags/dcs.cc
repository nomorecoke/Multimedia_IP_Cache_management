#include "debug/CacheRepl.hh"
#include "mem/cache/tags/dcs.hh"
#include "mem/cache/base.hh"

#define DCS_DEBUG 0

DCS::DCS(const Params *p)
    : BaseSetAssoc(p),
      updatePolicy(*this)
{

    DCS_numset = numSets;
    
    // Choose the leaders among entire set
    SetMonitor();
    
    // Miss counter for leaderset
    for(int ii = 0; ii < DCS_TYPE_NUM; ii++)
        DCS_MissCounter[ii] = 0;
    
    // Peridocally update policy
    schedule(updatePolicy,1000);
}


void
DCS::RecordHit(uint32_t set){
    
    //TODO hit promotion : LRU do nothing 
    
    if(DCS_monitor[set] != DCS_FOLLOWERS){

    }

}

void
DCS::RecordMiss(uint32_t set){
    //TODO miss promotion : LRU do nothing
    
    // if miss occurred in leader set 
    if(DCS_monitor[set] != DCS_FOLLOWERS){
        // count the miss in monitor
        DCS_MissCounter[DCS_monitor[set]-1]++;
    }
}

void
DCS::update_policy(void)
{
    cout << "time elapsed" <<curTick()/1000000000 << "M Cycle" << endl;
    getSegmentSize(0, 1);
    schedule(updatePolicy,curTick()+ 5000000000);
}

int
DCS::isAvailable( Addr addr,bool is_secure,MasterID master_Id ){

    string master_name = cache->system->getMasterName(master_Id);
    if(master_name.find("governor") == 0) { // Block is from the one of the multimedia IP

        Addr tag = extractTag(addr);
        int set = extractSet(addr); 
        int seg_size;
        int protect_count = 0 ;
        BlkType *blk = sets[set].findBlk(tag, is_secure);


        //hit --> available
        if(blk != nullptr)
            return 1;

        // miss
        else{
            for(int ii = 0 ; ii < assoc ; ii++){
                blk = sets[set].blks[ii];
                if(blk->isProtected)
                    protect_count++; 
            }
            seg_size = getSegmentSize(set,0);
            if(seg_size <= protect_count)
                return 0; // not avaliable
            else
                return 1; //available
        }
    }
    else 
        return 1;
}


CacheBlk* 
DCS::accessBlock( Addr addr, bool is_secure, Cycles &lat,MasterID master_Id)
{

    // search the tag directory with address. if hit return the block else return nullptr
    CacheBlk *blk = BaseSetAssoc::accessBlock(addr, is_secure, lat, master_Id);

    /* In case of hit, we will keep the information up-to-date
     * 1. Update the monitor as cache hit
     * 2. Hit promotion
     * 2.1. CPU block - set the reference bit and move the block to MRU position
     * 2.2. IP lbock - the frame data is passed to next ip, 
     *                 so we release the protection bit
     *                 we don't need th set the reference bit 
     */
    if (blk != NULL) {
        // Update monitor as the hit occurred
        RecordHit(blk->set);
        // Hit pomotion
        if(blk->isIP) { // If block is from IP
            blk->isProtected = 0; // clear the protection bit
            blk->reference_bit = 0;
            blk->rrpv = elapsedRRPV;
        }
        // CPU hit promotion set rrpv value to zero 
        else { // If block is from CPU
            blk->reference_bit = 1; // set the protection bit 
            blk->rrpv = 0;
        }
    }

    return blk;
}


CacheBlk*
DCS::findVictim(Addr addr, MasterID master_id)
{
    BlkType *blk = NULL;
    int set = extractSet(addr);
    int non_reference_count=0;
    int segment_size  = 0;
    int insertedBlockType;

    string master_name;

    // Classify the block's source master IP - where is from?
    master_name = cache->system->getMasterName(master_id);
    if(master_name.find("governor") == 0) { // Block is from the one of the multimedia IP
//       cout << master_name  << endl;
        insertedBlockType = 1;
    }
    else {   // Block is from Cpu_Cluster
        insertedBlockType = 0;
    }

#if DCS_DEBUG
    cout << cache->system->numRunningContexts()<<endl;
    else if(master_name.find("cpus") < 50){
        cout << "CPU number is" << master_name[master_name.find("cpus") + 4]<<endl;
    }
    for(int i = 0; i < cache->system->maxMasters(); i++) {
        cout << "Master ID("<<i<<"): " << cache->system->getMasterName(i) << endl;
    }
#endif

    /* Count the number of non-reference block */
    for (int ii = 0; ii < assoc; ii++){
        BlkType *b =sets[set].blks[ii];
        
        if(b->reference_bit == 0)   // if block is non-reference
            non_reference_count++;
        
        if(!b->isValid())      // if block is invalid
            return b;
        
        // keep going to find invalid block more closer to NRU position
        // Q: a new block is inserted at MRU postion whether found the more NRU closer block or not.
    }

    
    segment_size = getSegmentSize(set,0);

    if (insertedBlockType == 0) {
        /* If the inserted block is from CPU 
         * If there is no empty-line for inserted block, now we try to find a victim
         * With this policy, we will find the block as follows
         * Case 1: # of non-references < detected segment size
         *         - in this case, there is too many block in reference list
         *         - We select the victim in reference list with NRU position
         * Case 2: # of non-references >= detected segment size
         *         - in this case, there is too many block in non-reference list
         *         - we select the victim in non-reference list with NRU position
         */
        // Case 1: find victim from reference list
        if (segment_size > non_reference_count) { 
            for (int ii = assoc - 1; ii >= 0; ii--) {
                BlkType* b = sets[set].blks[ii];
                if (b->reference_bit){
                    if(b->rrpv == elapsedRRPV){
                        return b;
                    }
                }

                if (ii == 0){
                    for(int jj = 0 ; jj <assoc; jj++){
                        if(b->reference_bit){
                            b->rrpv++;
                        }
                    }
                    ii = assoc - 1;
                }
            }
        }
        // Case 2: find victim from non reference list
        else { 
            for (int ii = assoc - 1; ii >= 0; ii--) {
                BlkType* b = sets[set].blks[ii];
                if ((!b->reference_bit)&&(b->isProtected == 0))
                    return b;
            }
            // all non-reference list is protected --> find victim from reference list
            for (int ii = assoc - 1; ii >=0; ii--){
                BlkType *b = sets[set].blks[ii];

                if (b->reference_bit){
                    if(b->rrpv == elapsedRRPV){
                        return b;
                    }
                }

                if (ii == 0){
                    for(int jj = 0 ; jj <assoc; jj++){
                        if(b->reference_bit){
                            b->rrpv++;
                        }
                    }
                    ii = assoc - 1;
                }
            }
        }
    }
    else {
        /*
         * If the inserted block is form IP
         * If there is no empty-line for inserted block, now we try to find a victim
         * from non-reference list only
         */
        for (int ii = assoc - 1; ii >= 0; ii--) {
            BlkType*b = sets[set].blks[ii];
            if ((!b->reference_bit)&&(b->isProtected == 0))
                return b;
        }

        if( segment_size > non_reference_count){
            for( int ii = assoc - 1; ii >= 0 ; ii--){
                BlkType * b = sets[set].blks[ii];

                if (b->reference_bit){
                    if(b->rrpv == elapsedRRPV){
                        return b;
                    }
                }

                if (ii == 0){
                    for(int jj = 0 ; jj <assoc; jj++){
                        if(b->reference_bit){
                            b->rrpv++;
                        }
                    }
                    ii = assoc - 1;
                }
            }
        }
    }
    return blk;
}


void
DCS::insertBlock(PacketPtr pkt, BlkType *blk)
{
    string master_name;
    BaseSetAssoc::insertBlock(pkt, blk);
    int set = extractSet(pkt->getAddr());

    master_name = cache->system->getMasterName(pkt->req->masterId());
    if(pkt->req->masterId() == 0)
        master_name = cache->system->getMasterName(pkt->req->oriMasterId());

    if(master_name.find("governor") == 0 ){
        blk->isIP = 1;
        blk->isProtected  = 1;
    }
    else {
        blk->isIP = 0;
        RecordMiss( set );
    }
    

    blk->reference_bit = 0; // set the non-reference bit
    blk->rrpv = elapsedRRPV - 1;
    sets[set].moveToHead(blk);
}

void
DCS::invalidate(CacheBlk *blk)
{
    BaseSetAssoc::invalidate(blk);
}

DCS*
DCSParams::create()
{
    return new DCS(this);
}

/*
INT32 DCS::Get_DCS_Victim( UINT32 setIndex)
{
}
*/

/* 
 *      Choose the Leader Sets
 */

/*
inline unsigned int dcs_rand(void){
   unsigned long dcs_rand_seed;
    dcs_rand_seed = dcs_rand_seed * 1103515245 +12345;
    return ((unsigned)(dcs_rand_seed/65536) % dcs_max_rand);
}

*/
void DCS::SetMonitor(void){
    DCS_monitor = (int *) calloc(DCS_numset, sizeof(int));
    for ( int ii = 0; ii < DCS_numset ;ii++)
        DCS_monitor[ii] = 0;

    RandomlyChooseLeader();
}


void DCS::RandomlyChooseLeader(void)
{
    unsigned long dcs_rand_seed = 1;
    for (int p = 0; p < DCS_TYPE_NUM; p++){
        for (int ii = 0; ii < DCS_Leader_set_num; ii++){
            unsigned int random_idx;
           // set random value fixded for experience 
           // TODO : seed change to time value or anythimgelse
            dcs_rand_seed = dcs_rand_seed * 1103515245 +12345;
            random_idx = ((unsigned)(dcs_rand_seed/65536) % DCS_numset);

            if(DCS_monitor[random_idx] == DCS_FOLLOWERS){
                   DCS_monitor[random_idx] = p + 1;
            }
            else ii--; // re choose the random index 

        }
    }

}

int DCS::getSegmentSize(uint32_t set, int isUpdate){

    int min = DCS_MissCounter[0]; // minimum miss value
    int min_seg = 0; // size num
    double threshhold;
    // set new segment size
    if(isUpdate == 1){
        for( int ii = 0; ii< DCS_TYPE_NUM ; ii++){
            if(min > DCS_MissCounter[ii]){
                min = DCS_MissCounter[ii];
                min_seg = ii;
            }
            cout << ii <<" : " << DCS_MissCounter[ii] << endl;
        }
        threshhold = (double)min * 1.05;

        for( int ii = 0; ii< DCS_TYPE_NUM ;ii++){ 
            if( (ii != min_seg) && (threshhold > DCS_MissCounter[ii])){
                min_seg = ii;
            }
            DCS_MissCounter[ii] = 0;
        }
        DCS_current_seg_size = segment_size_array[min_seg];
        //if(min_seg != 1)
         cout << "<<<< segment size changed ( " << DCS_current_seg_size <<" )"<< endl;
         segment_sum +=DCS_current_seg_size;
         segment_count ++;
         cout << "Average Segment size" << (double)segment_sum /segment_count << endl;
    }
    else{   // return current segment size
        if(DCS_monitor[set] == DCS_FOLLOWERS)
            return DCS_current_seg_size;
        /* for leader set */
        else
            return segment_size_array[DCS_monitor[set]-1];
    }
    return 0;
}
