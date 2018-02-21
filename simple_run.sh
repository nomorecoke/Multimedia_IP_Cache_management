#! /bin/bash

GEM5_DIR=/home/yongwoo/test/gem5
    mkdir $2
    cd $2
    mkdir $i
    cd ..
    time $GEM5_DIR/build/ARM/gem5.opt -d "$2/$1" $GEM5_DIR/configs/example/arm/lg_ip_se.py \
        --mem-type=LPDDR3_1600_1x32 \
        --mem-channels=1 \
        --cpu="minor" \
        --add_IP --cache_mapping --IP_cfg_file=$GEM5_DIR/configs/example/arm/template.cfg \
        --num-cores=1  --l2_policy=PDPP $1 2>&1 | tee -i ./$2/$1/$1.out 
    


