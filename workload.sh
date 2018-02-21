#! /bin/bash
k=1
GEM5_DIR=/home/yongwoo/test/gem5
W=(
    milc libquantum sjeng
    lbm astar perlbench
    wrf perlbench hmmer
    astar sjeng bwaves
    calculix bzip2 povray
    )
POLICY=(
    LRU
    DCS
    PDPP
    )

cd WORKLOAD_IP15
for ((i = 0; i < 15 ; i=i+3))
do
    
    mkdir WORKLOAD$k
    cd WORKLOAD$k
    k=$(($k+1))
    
    for j in ${POLICY[1]}
    do
        mkdir $j
        time $GEM5_DIR/build/ARM/gem5.opt -d "$j" $GEM5_DIR/configs/example/arm/lg_ip_se.py \
            --mem-type=LPDDR3_1600_1x32 \
            --mem-channels=1 \
            --cpu="minor" \
            --add_IP --cache_mapping --IP_cfg_file=$GEM5_DIR/configs/example/arm/template_slice.cfg \
            --num-cores=3 --l2_policy=$j ${W[$i]} ${W[$i+1]} ${W[$i+2]}  2>&1 | tee -i $j/$j.out &
    done
    
    cd ..
done

#--cache_mapping


