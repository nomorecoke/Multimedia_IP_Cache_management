#include "debug/CacheRepl.hh"
#include "mem/cache/tags/pdp_seg.hh"
#include "mem/cache/base.hh"

#define SIMPLE_CALC(nii,ni,nt,d,W) (double)(nii+((nt-ni)*(d+W))) == 0 ? 0 :  (double)ni/(double)(nii+((nt-ni)*(d+W)))
PDPS::PDPS(const Params *p)
    : BaseSetAssoc(p),
      updatePolicy(*this)
{
    PDP_numset = numSets;
    cout <<"# of Cache set"<<PDP_numset << endl;
    SetMonitor();
//    RandomlyChooseLeader();
    for(int ii = 0; ii < PDP_TYPE_NUM; ii++)
        PDP_MissCounter[ii] = 0;
//    PDP = new DIP( "LLC", 0, 4, getNumSets(),
 //           USE_INSERTION_POLICY, 1024, USE_SDMsize);
    schedule(updatePolicy,1000);
}


void
PDPS::RecordHit(uint32_t set){
    //TODO hit promotion : LRU do nothing
    //leader set
//    cout <<"cache hit!! " << endl;

}

void
PDPS::RecordMiss(uint32_t set){
    //TODO miss promotion : LRU do nothing

    for (int ii = 0; ii < assoc; ii++){
        BlkType *b =sets[set].blks[ii];
        b->RD--;
        if(b->RD == 0){
            b->RD = MAX_RD + b->originalPD;
            b->isProtected = 0;
        }
    }

    //leader set
    if(PDP_monitor[set] != PDP_FOLLOWERS){
//        cout<< "leader cache miss!!"<< endl;
        PDP_MissCounter[PDP_monitor[set]-1]++;
    }
}

void
PDPS::update_policy()
{
    cout << "time elapsed" << curTick()/1000<<"Cycle" << endl;
    PD_Compute_Logic();
    schedule(updatePolicy, curTick() + 5000000000); // 50m cycle
}






CacheBlk* 
PDPS::accessBlock( Addr addr, bool is_secure, Cycles &lat,MasterID master_id)
{
    CacheBlk *blk = BaseSetAssoc::accessBlock(addr, is_secure, lat,master_id);

    RDSampler(addr,master_id);

    if (blk != NULL) { //cache hit !!!!!!!!!!!
        RecordHit(blk->set);
        // multimedia IP access hit promotion : unprotect block
        if(blk->isIP)
            blk->isProtected = 0;
        else{
            blk->isProtected = 1;
            blk->RD=blk->originalPD;
            blk->reference_bit  = 1;  //hit promotion TODO: segment bit changed to 1
        }
    }
    return blk;
}


CacheBlk*
PDPS::findVictim(Addr addr, MasterID master_id)
{
    BlkType *blk = NULL;
    int set = extractSet(addr);
#if 0
    int non_reference_count=0;
    int segment_size  = 0;
    int victim_segment =0;
    int cpu_num = 0 ;
#endif
    int victim_IP = 0;
    int victim_index;
    int max_rd;
    string master_name; // get master name from master_id

    master_name = cache->system->getMasterName(master_id);
    // is IP return 0
    if(master_name.find("governor")==0){
        cout << " this is IP " << endl;
        victim_IP = 1;
    }

    for (int ii = 0; ii < assoc; ii++){
        BlkType *b =sets[set].blks[ii];
        if(!b->isValid()){ //allocate invalid first
            return b;
        }
    }
    if(victim_IP == 1){
        //TODO: add it later 11-21
    }

    /* victim selector of PDP */
    victim_index = -1; 
    max_rd = 0; 
    for (int ii = 0; ii < assoc; ii++){
        BlkType *b = sets[set].blks[ii];
        if((b->isProtected == 0) && (b->RD > max_rd)){
            max_rd = b->RD;
            victim_index = ii ;
        }
    }
    // every block is protected
    if(victim_index == -1){
        for (int ii = 0; ii < assoc; ii++){
            BlkType *b = sets[set].blks[ii];
            if((b->RD > max_rd)){
                max_rd = b->RD;
                victim_index = ii ;
            }
        }
    }
    blk = sets[set].blks[victim_index];

    assert(blk != NULL);
    //TODO : IP access and everey block is protected -> Bypass
    return blk;
}


void
PDPS::insertBlock(PacketPtr pkt, BlkType *blk)
{
    string master_name;
    BaseSetAssoc::insertBlock(pkt, blk);
    int set = extractSet(pkt->getAddr());
    int cpu_num;


    
    master_name=cache->system->getMasterName(pkt->req->masterId());
    if(pkt->req->masterId() ==0)
        master_name=cache->system->getMasterName(pkt->req->oriMasterId());
    //IP
    if(master_name.find("governor") == 0 ){
        blk->isIP = 1;
        blk->isProtected  = 1;
    }

    //CPU
    else if(master_name.find("cpus") <50){ 
        cpu_num= master_name[ master_name.find("cpus")+4];
        if(cpu_num==46) cpu_num = 0;
        else cpu_num= cpu_num -'0';
        blk->isProtected = 0;
        blk->isIP = 0;
        blk->RD = opt_PD[cpu_num];
        blk->originalPD = opt_PD[cpu_num];
    }

    blk -> reference_bit = 0; 
    RecordMiss( set );
}

void
PDPS::invalidate(CacheBlk *blk)
{
    BaseSetAssoc::invalidate(blk);
}

PDPS*
PDPSParams::create()
{
    return new PDPS(this);
}

/*
INT32 PDPS::Get_PDP_Victim( UINT32 setIndex)
{
}
*/

/* 
 *      Choose the Leader Sets
 */

/*
inline unsigned int PDP_rand(void){
   unsigned long PDP_rand_seed;
    PDP_rand_seed = PDP_rand_seed * 1103515245 +12345;
    return ((unsigned)(PDP_rand_seed/65536) % PDP_max_rand);
}

*/
void PDPS::SetMonitor(void){
    PDP_monitor = (int *) calloc(PDP_numset, sizeof(int));
    for ( int ii = 0; ii < PDP_numset ;ii++)
        PDP_monitor[ii] = 0;
    
    

    RandomlyChooseLeader();
}


void PDPS::RandomlyChooseLeader(void)
{
    unsigned long PDP_rand_seed = 1;
    for (int p = 0; p < PDP_TYPE_NUM; p++){
        for (int ii = 0; ii < PDP_Leader_set_num; ii++){
            unsigned int random_idx;
           // set random value fixded for experience TODO : seed change to time value or anythimgelse
            
            PDP_rand_seed = PDP_rand_seed * 1103515245 +12345;
            random_idx = ((unsigned)(PDP_rand_seed/65536) % PDP_numset);

            if(PDP_monitor[random_idx] == PDP_FOLLOWERS){
                   PDP_monitor[random_idx] = p + 1;
//                   cout << "this set ("<<random_idx<<") is leader set of size" << p*4 << endl;
            }
            else ii--; // re choose the random index 

        }
    }

}

int PDPS::getSegmentSize(uint32_t set, int isUpdate){
    int min = PDP_MissCounter[0]; // minimum miss value
    int min_seg = 1 ; // size num

    // set new segment size
    if(isUpdate == 1){ // TODO : call this periodically 
        for( int ii = 0; ii< PDP_TYPE_NUM ; ii++){
            if(min > PDP_MissCounter[ii])
            {
                min = PDP_MissCounter[ii];
                min_seg = ii;
            }
            //           cout << ii+1 <<" : " << PDP_MissCounter[ii] << endl;
            //            PDP_MissCounter[ii] = 0;
            PDP_current_seg_size = segment_size_array[min_seg];
            //if(min_seg != 1)
            //  cout << "<<<< segment size changed ( " << PDP_current_seg_size <<" )"<< endl;
        }



        // return current segment size
        if(PDP_monitor[set] == PDP_FOLLOWERS)
            return PDP_current_seg_size;

        /* for leader set */
        else
            return segment_size_array[PDP_monitor[set]-1];
    }
    else return PDP_current_seg_size;
}
/*this is for PDP sampler*/

void PDPS::RDSampler(Addr addr,MasterID master_Id){
    int set = extractSet(addr);
    int tag = extractTag(addr);
    int cpu_num=0;
    int find = 0 ;
    int index = 0;
    string master_name;
    
    /* for initialize */
    if(RD_init == 0){
        num_of_CPU = cache->system->numRunningContexts();
        
        opt_PD = new int[num_of_CPU];
        set_counter= new int[PDP_numset+1];
        for(int ii = 0; ii < PDP_numset+1 ; ii++)
            set_counter[ii] = 0;



        rd_counter = new int*[num_of_CPU];
        for(int ii = 0 ; ii <num_of_CPU; ii++){
            opt_PD[ii] = 1 ;
            rd_counter[ii] = new int[MAX_RD];
            for(int jj = 0; jj < MAX_RD; jj++)
                rd_counter[ii][jj] = 0;
        }
        cout << "num of CPU " << num_of_CPU << endl;
        RD_init = 1;
    }


    /* check for CPU number */    
    master_name=cache->system->getMasterName(master_Id);
    if(master_name.find("cpus") <50  ){ 
       cpu_num= master_name[ master_name.find("cpus")+4];
       if(cpu_num==46) cpu_num = 0;
       else cpu_num= cpu_num -'0';
    }
    else return;

    //this is for RD smapler//
    set_counter[set]++;
    for ( int ii = 0; ii < sampler.size(); ii++){
        find = 0;
        RD_sampler temp = sampler[ii];
        if((temp.set == set) &&(temp.tag == tag) && (temp.cpu == cpu_num)){ // hit in sampler, count up 
            find = 1;
            index = ii;
            break;
        }
    }

    if(find){ // hit 
        int rd = set_counter[set] - sampler[index].last_access;
        
        if(rd >= MAX_RD) rd= MAX_RD;
        
        
        rd_counter[cpu_num][rd-1]++;

        sampler[index].last_access = set_counter[set];
//        cout << "Reuse distance update (" << cpu_num <<" ) :" << rd << ":" << rd_counter[cpu_num-1][rd-1]<<endl;
//        PD_Compute_Logic();
    }
    else{ // miss add to sampler
        RD_sampler temp = RD_sampler(set,tag, cpu_num, set_counter[set]); 
        sampler.push_back(temp);
    }
}

void PDPS::PD_Compute_Logic(void){
    int Nt;  // Nt is total acceess
    // Ni is sum of access from  i to dp 
    int sum_Ni_i; // sum of Ni * i
    int sum_Ni; // sum of Ni
    int dp;
    double E_now;
    double E_before;
    for ( int ii = 0 ; ii < num_of_CPU ; ii++){
        // initialize
        Nt = 0; sum_Ni_i = 0;  sum_Ni =0; dp =0; E_now = 0; E_before = 0;
        // calculate Nt 
        for ( int jj = 0; jj < MAX_RD ; jj++){
            Nt += rd_counter[ii][jj];
        }
        if(Nt == 0) continue;
//        cout << Nt << endl;
        //calculate PD , dp from 1 to MAX_RD
        for ( int jj = 0; jj < MAX_RD ; jj++){
            sum_Ni_i += (rd_counter[ii][jj]*(jj+1));
            sum_Ni += rd_counter[ii][jj];
            dp = jj+1;
            E_now = SIMPLE_CALC(sum_Ni_i,sum_Ni,Nt,dp,assoc);
            if(E_before >= E_now && E_now!= 0) break; 
            E_before = E_now;
        }
        
        // dp is optimal PD, and E_before is HR(dp)/W
        opt_PD[ii] = dp;
        cout << "CPU " << ii << " PD :" << dp<<"<<<<<<"<<E_before <<endl;
    }
}



