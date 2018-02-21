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

cd 1127_DCS
for i in ${SPEC_BENCH[*]}
do
    mkdir $i
    $GEM5_DIR/build/ARM/gem5.opt -d "$i" $GEM5_DIR/configs/example/arm/lg_ip_se.py \
        --mem-type=LPDDR3_1600_1x32 \
        --mem-channels=1 \
        --cpu="minor" \
        --add_IP --cache_mapping --IP_cfg_file=$GEM5_DIR/configs/example/arm/template.cfg
        --num-cores=1  --l2_policy=DCS $i 2>&1 | tee -i ./$i/$i.out &
done
cd ..


