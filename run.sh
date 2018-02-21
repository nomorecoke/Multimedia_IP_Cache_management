#! /bin/sh

GEM5_DIR=/home/yongwoo/test/gem5/
RESULT_DIR=$1

scons build/ARM/gem5.opt -j16
#cd $1
./build/ARM/gem5.opt -d "$RESULT_DIR" $GEM5_DIR/config/example/arm/lg_ip_se.py \
    --mem-type=LPDDR3_1600_1x32 \
    --mem-channels=1 \
    --cpu="minor" \
    --num-cores=3 \
    $2 $3 $4
cd $GEM5_DIR

