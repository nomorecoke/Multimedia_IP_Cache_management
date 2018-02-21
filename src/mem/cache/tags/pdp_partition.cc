#include "debug/CacheRepl.hh"
#include "mem/cache/tags/pdp_partition.hh"
#include "mem/cache/base.hh"

#define SIMPLE_CALC(nii,ni,nt,d,W) (double)(nii+((nt-ni)*(d+W))) == 0 ? 0 :  (double)ni/(double)(nii+((nt-ni)*(d+W)))
PDPP::PDPP(const Params *p)
    : BaseSetAssoc(p),
      updatePolicy(*this)
{
    PDP_numset = numSets;
    cout <<"# of Cache set"<<PDP_numset << endl;
//    RandomlyChooseLeader();
//    PDP = new DIP( "LLC", 0, 4, getNumSets(),
 //           USE_INSERTION_POLICY, 1024, USE_SDMsize);
 //
    schedule(updatePolicy, 1000);
}

int
PDPP::isAvailable( Addr addr,bool is_secure,MasterID master_Id ){

    string master_name = cache->system->getMasterName(master_Id);
    if(master_name.find("governor") == 0) {

        Addr tag = extractTag(addr);
        int set = extractSet(addr); 
        int protect_count = 0 ;
        BlkType *blk = sets[set].findBlk(tag, is_secure);

        if(blk != nullptr)
            return 1;
        else{
            for ( int ii = 0; ii< assoc; ii++){
                BlkType *b = sets[set].blks[ii];
                if(!b->isValid()){
                    return 1;
                }
                if(b->isProtected)             
                    protect_count++;
            }

            if(protect_count == assoc)
                return 0;
        }
    }

    return 1;
}

void
PDPP::RecordHit(uint32_t set){
    //TODO hit promotion : LRU do nothing
    //leader set
//    cout <<"cache hit!! " << endl;

}

void
PDPP::RecordMiss(uint32_t set){
    //TODO miss promotion : LRU do nothing

    for (int ii = 0; ii < assoc; ii++){
        BlkType *b =sets[set].blks[ii];
        b->RD--;
        if(b->RD <= 0){
            b->RD = MAX_RD + b->originalPD;
            b->isProtected = 0;
        }
    }

}

void
PDPP::update_policy()
{
    /*
     * Do Something
     */
    cout <<"time elapsed" << curTick()/1000 << "Cycle" << endl;

    PD_Compute_Logic();
    schedule(updatePolicy,curTick()+ 5000000000);
}



CacheBlk* 
PDPP::accessBlock( Addr addr, bool is_secure, Cycles &lat,MasterID master_id)
{
    CacheBlk *blk = BaseSetAssoc::accessBlock(addr, is_secure, lat,master_id);

    RDSampler(addr,master_id);
    if (blk != NULL) { //cache hit !!!!!!!!!!!
        // multimedia IP access hit promotion : unprotect block
        if(blk->isIP){
            blk->isProtected = 0;
            blk->isIP = 0;
            blk->RD=MAX_RD;

        }
        else{
            blk->isProtected = 1;
            blk->RD=opt_PD[blk->cpuNum];
            blk->reference_bit  = 1;  //hit promotion TODO: segment bit changed to 1
        }
    }
    return blk;
}


CacheBlk*
PDPP::findVictim(Addr addr, MasterID master_id)
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
    int victim_index=-1;
    int max_rd;
    int cpu_num = - 1;
    
    string master_name; // get master name from master_id

    master_name = cache->system->getMasterName(master_id);
    // is IP return 0
    if(master_name.find("governor")==0){
        victim_IP = 1;
    }


    
    if(victim_IP == 1){
        int ip_alloc;
        ip_alloc = assoc - allocationWay[0];

        for ( int ii = 0; ii< assoc; ii++){
            BlkType *b = sets[set].blks[ii];
            if(!b->isValid()){
                return b;
            }
            if(b->isIP == 1)
                ip_alloc--;   
        }
        //can allocation 
        if(ip_alloc > 0){
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
        }
        else
            cout << "error : need to modify available function" << endl;
            

        //TODO: add it later 11-21
    }
    else{
        occupied[cpu_num] = allocationWay[cpu_num];

        for (int ii = 0; ii < assoc; ii++){
            BlkType *b =sets[set].blks[ii];
            if(!b->isValid()){ //allocate invalid first
                return b;
            }
            
            if(b->isIP == 0) 
                occupied[b->cpuNum]--;
            
        }

        //less occupied :: victim can be anything
        if(occupied[cpu_num] > 0){
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
        }
        //over occupied :: victim is my cpu's block
        else{
            /* victim selector of PDP */
            victim_index = -1; 
            max_rd = 0; 
            for (int ii = 0; ii < assoc; ii++){
                BlkType *b = sets[set].blks[ii];
                if((b->isProtected == 0) && (b->RD > max_rd) && (b->cpuNum == cpu_num)){
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
        }
    }


    blk = sets[set].blks[victim_index];

    assert(blk != NULL);
    //TODO : IP access and everey block is protected -> Bypass
    return blk;
}


void
PDPP::insertBlock(PacketPtr pkt, BlkType *blk)
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
        blk->cpuNum = -1 ;
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
        blk->cpuNum = cpu_num;
    }


    blk -> reference_bit = 0; 
    RecordMiss( set );
}

void
PDPP::invalidate(CacheBlk *blk)
{
    BaseSetAssoc::invalidate(blk);
}

PDPP*
PDPPParams::create()
{
    return new PDPP(this);
}


void PDPP::RDSampler(Addr addr,MasterID master_Id){
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

        allocationWay = new int(num_of_CPU);
        tempWay = new int(num_of_CPU);
        occupied = new int(num_of_CPU);

        for( int ii = 0; ii< num_of_CPU ; ii++){
            allocationWay[ii] = (int)assoc/num_of_CPU;
            cout << "initial alloc"<<allocationWay[ii] << endl;
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
        
        
        rd_counter[0][rd-1]++;

        sampler[index].last_access = set_counter[set];
//        cout << "Reuse distance update (" << cpu_num <<" ) :" << rd << ":" << rd_counter[cpu_num-1][rd-1]<<endl;
        //PD_Compute_Logic();
    }
    else{ // miss add to sampler
        RD_sampler temp = RD_sampler(set,tag, cpu_num, set_counter[set]); 
        sampler.push_back(temp);
    }
}

void PDPP::PD_Compute_Logic(void){
    int Nt;  // Nt is total acceess
    // Ni is sum of access from  i to dp 
    int sum_Ni_i; // sum of Ni * i
    int sum_Ni; // sum of Ni
    int dp;
    double E_now;
    double E_before;
    for ( int jj = 0 ; jj <MAX_RD ;jj++)
        for ( int ii = 1 ; ii <num_of_CPU ;ii++)
            rd_counter[0][jj]+=rd_counter[ii][jj];

    for ( int ii = 0 ; ii < 1 ; ii++){
        // initialize
        Nt = 0; sum_Ni_i = 0;  sum_Ni =0; dp =0; E_now = 0; E_before = 0;
        // calculate Nt 
        for ( int jj = 0; jj < MAX_RD ; jj++){
            Nt += rd_counter[ii][jj];
        }
        if(Nt == 0){
            allocationWay[ii] = 1;
            continue; 
        }
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
        // for exception
        if(E_before <= 0 ) tempWay[ii] = 1;
        else  allocationWay[ii] = PD_Way_Calculate(E_before);
#if 1
        cout << "CPU CLUSTER" << ii << " PD :" << dp<<"<<<<<<"<<E_before <<endl;
#endif
    }



    if(assoc < allocationWay[0])
        allocationWay[0] = assoc;

    for ( int ii = 0; ii < num_of_CPU; ii++){
        opt_PD[ii] = opt_PD[0];
        allocationWay[ii] = allocationWay[0];
        cout <<"           way of CPU #"<<ii<<"  :   "<<allocationWay[ii] << endl;
        //cout <<"CPU ( " <<ii<<") PD : "<< dp <<  " way : " << allocationWay[ii] << " E "<< E_before<<endl;

    }


}

int PDPP::PD_Way_Calculate(double E){
    int ii;
    for(ii = 0; ii< assoc; ii++)
       if( E * ii >= 1) break;

    return ii+1;
}

