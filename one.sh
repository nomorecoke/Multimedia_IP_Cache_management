#! /bin/bash

GEM5_DIR=/home/yongwoo/test/gem5

SPEC_BENCH=(
    perlbench
    bzip2
    gcc
    bwaves
    gamess
    mcf
    milc
    zeusmp
    gromacs
    cactusADM
    leslie3d
    namd
    gobmk
    soplex
    povray
    calculix
    hmmer
    sjeng
    GemsFDTD
    libquantum
    h264ref
    tonto
    lbm
    omnetpp
    astar
    wrf
    sphinx3
    )

for i in ${SPEC_BENCH[*]}
do
    rm -rf $i
#    $GEM5_DIR/build/ARM/gem5.opt -d "$i" $GEM5_DIR/configs/example/arm/lg_ip_se.py \
#        --mem-type=LPDDR3_1600_1x32 \
#        --mem-channels=1 \
#        --cpu="minor" \
 #       --num-cores=1 --l2_policy=DCS $i 2>&1 | tee -i debugdcs.out
done


