# /bin/bash

GEM5_DIR=/home/yongwoo/test/gem5

SPEC_BENCH=(
    perlbench
    bzip2
    gcc
    bwaves
    gamess
    )


JOOSUCK=(
    mcf
    milc
    zeusmp
    gromacs
    cactusADM
    )


JOOSUK=(    
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
    $GEM5_DIR/build/ARM/gem5.opt -d "max_hit_DDR4/$i" $GEM5_DIR/configs/example/arm/lg_ip_se.py \
        --mem-type=DDR4_2400_4x16 \
        --mem-channels=2 \
        --cpu="minor" \
        --num-cores=1 --l2_policy=LRU $i 2>&1 | tee -i max_hit_log.out &
done


