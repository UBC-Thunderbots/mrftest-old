#!/bin/bash

#=============================================================================#
# This is a first-time installation script for JetBrain's CLion IDE.
# It automatically downloads and unpacks CLion to the /usr/share directory
# and links the exetuable so it can be found in the user's PATH. It also
# automcatically runs CLion so the first-time-setup can be performed 
# (such as selecting theme and toolchains)
#=============================================================================#

echo "================================================================"
echo "Installing CLion"
echo "================================================================"
CLION_VERSION="2017.3.3"

# Install dependencies
sudo apt-get install -y openjdk-8-jdk

# Fetch and extract CLion
echo "Fetching and extracting CLion"
wget https://download.jetbrains.com/cpp/CLion-$CLION_VERSION.tar.gz
sudo rm -rf /usr/share/clion-$CLION_VERSION
sudo tar xzf CLion-$CLION_VERSION.tar.gz -C /usr/share
rm -rf CLion-$CLION_VERSION.tar.gz

# Run CLion Setup
cd /usr/share/clion-$CLION_VERSION
./bin/clion.sh

# Make CLion globally accessible
# This makes a link from the CLion executable to /usr/local/bin/
# which is in the system $PATH by default and by convention holds
# user-installed executables. Making this link allows CLion to be run
# from anywhere on the command-line.
echo "Linking CLion"
sudo rm -rf /usr/local/bin/clion
sudo ln -s -f /usr/share/clion-$CLION_VERSION/bin/clion.sh /usr/local/bin/clion
