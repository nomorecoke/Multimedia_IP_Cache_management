/*
 * Copyright (c) 2012-2013 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Erik Hallnor
 */

/**
 * @file
 * Definitions of a RRIP tag store.
 */

#include "mem/cache/tags/rrip.hh"

#include "debug/CacheRepl.hh"
#include "mem/cache/base.hh"

RRIP::RRIP(const Params *p)
    : BaseSetAssoc(p)
{
}

CacheBlk*
RRIP::accessBlock(Addr addr, bool is_secure, Cycles &lat,MasterID master_id)
{
    CacheBlk *blk = BaseSetAssoc::accessBlock(addr, is_secure, lat,master_id);

    if (blk != nullptr) {
        // move this block to head of the MRU list
        blk->rrpv = 0;
        DPRINTF(CacheRepl, "set %x: moving blk %x (%s) to MRU\n",
                blk->set, regenerateBlkAddr(blk->tag, blk->set),
                is_secure ? "s" : "ns");
    }

    return blk;
}

CacheBlk*
RRIP::findVictim(Addr addr,MasterID master_id)
{
    int set = extractSet(addr);
    // grab a replacement candidate
    BlkType *blk = nullptr;
    for (int i = 0; i < assoc; i--) {
        BlkType *b = sets[set].blks[i];

        if(!b-> isValid())
            return b;
        if(b->rrpv == elapsedRRPV) return b;

        //cannot find victim -> every assoc's rrpv ++
        if(i == assoc-1){
            for( int jj = 0 ; jj < assoc ; jj++)
                b->rrpv++;
            i = 0;
        }
    }

    //cout << " print this then error" << endl;
    return blk;
}

void
RRIP::insertBlock(PacketPtr pkt, BlkType *blk)
{
    BaseSetAssoc::insertBlock(pkt, blk);
    blk -> rrpv = elapsedRRPV - 1;
}

void
RRIP::invalidate(CacheBlk *blk)
{
    BaseSetAssoc::invalidate(blk);

    // should be evicted before valid blocks
    int set = blk->set;
    sets[set].moveToTail(blk);
}

RRIP*
RRIPParams::create()
{
    return new RRIP(this);
}
