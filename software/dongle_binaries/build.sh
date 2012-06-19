#!/bin/bash

build_for_channel()
{
	if [ -z "$1" ]
	then
		echo "Pass channel as first param"
	else
		rm -rf obj/mrf && CXXFLAGS="-DCHANNEL=$1" make -j 3 bin/mrftest && mv bin/mrftest dongle_binaries/mrftest_$1
		rm -rf obj/fw && CXXFLAGS="-DCHANNEL=$1" make -j 3 bin/fw && mv bin/fw dongle_binaries/fw_$1
	fi
}

DIR=`pwd`
cd ..

build_for_channel 11
build_for_channel 12
build_for_channel 20
build_for_channel 21

cd $DIR
