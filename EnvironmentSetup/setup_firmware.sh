#!/usr/bin/env bash

# Setup script to get the arm toolchains for our firmware code.

# Link to the instructions that basically layout how this bash script works:
#   https://gnu-mcu-eclipse.github.io/toolchain/arm/install/

# temporary setup folder to download and unpack tarball
TEMP_FOLDER=temp-gcc-setup
# output name for tarball
OUTPUT_TAR=gcc-arm-none-eabi-7.tar.bz2
# the link to the arm-none-eabi tarball
# got the link here: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
GCC_LINK=https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2017q4/gcc-arm-none-eabi-7-2017-q4-major-linux.tar.bz2?revision=375265d4-e9b5-41c8-bf23-56cbe927e156?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Linux,7-2017-q4-major
# the folder name of the unpacked tarball
UNPACKED_TAR=gcc-arm-none-eabi-7-2017-q4-major
# location to store the toolchains
TBOTS_TOOLCHAIN=~/tbots-toolchains/

cd
# create the folder to store the toolchain
mkdir -p $TBOTS_TOOLCHAIN
# create a temporary setup folder
mkdir -p $TEMP_FOLDER
# go into the temporary setup folder
cd $TEMP_FOLDER

# download the arm-none-eabi
wget -O $OUTPUT_TAR $GCC_LINK
# unpack it
tar xjf $OUTPUT_TAR
# change permissions
chmod -R -w $UNPACKED_TAR

# go into unpacked arm-none-eabi
cd $UNPACKED_TAR
# move all of the folders to our tbots toolchain location
mv * $TBOTS_TOOLCHAIN

cd
# remove the temporary setup folder
rm -r $TEMP_FOLDER


