#!/bin/bash

build_for_channel()
{
	if [ -z "$2" ]
	then
		echo "Pass channel as first param, pan_id as second"
	else
		rm -rf obj/mrf && CXXFLAGS="-DCHANNEL=$1 -DPAN_ID=$2" make -j 3 bin/mrftest && mv bin/mrftest dongle_binaries/mrftest_$1_$2
		rm -rf obj/fw && CXXFLAGS="-DCHANNEL=$1 -DPAN_ID=$2" make -j 3 bin/fw && mv bin/fw dongle_binaries/fw_$1_$2
	fi
}

DIR=`pwd`
cd ..

build_for_channel 20 0x1846
build_for_channel 21 0x1439
build_for_channel 21 0x1387

cd $DIR
