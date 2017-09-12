#!/bin/bash
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# UBC Thunderbots Linux Environment Setup
#
# This script will install all the required libraries and dependencies to build
# and run the Thunderbots codebase.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#

# First, we test whether bash supports arrays.
# (Support for arrays was only added recently.)
# From http://tldp.org/LDP/Bash-Beginners-Guide/html/sect_10_02.html
testarray[0]='test' || (echo 'Failure: arrays not supported in this version of
bash.' && exit 2)

# Make sure that we are running as root
if [ "$EUID" -ne 0 ]
  then echo "Please run as root via the \`sudo\` command"
  exit 2
fi

# Save the parent dir of this script since we want to return here
# so that commands are run relative to the location of this script
parent_path=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
cd $parent_path

# Install packages required to run the HOST SOFTWARE
host_software_packages=(
    protobuf-compiler           # Google protocol buffers
    libprotobuf-dev             # Google protocol buffers
    g++                         # The C++ compiler
    libgtkmm-3.0-dev            # gtkmm-3.0, used to create the gui?
    libode-dev                  # libode-dev
    libxml++2.6-dev             # libxml++
    libgsl0-dev                 # libgsl0-dev
    libusb-1.0.0-dev            # libusb-1.0.0-dev
    libcppunit-dev              # CPPUnit, the unit testing framework
    libboost-coroutine1.54-dev  # libboost-coroutine-dev, the coroutines library
    libbz2-dev                  # bzip2, used for archiving and storing our log files
)

# Update sources
apt-get update

# Install `software-properties-common` that provides the `add-apt-repository`
# command used later on in this script
apt-get install software-properties-common -y

# This if-block will add the required repositories to install coroutines-1.54
# The full entry that must be added to the list of sources is
# http://ca.archive.ubuntu.com/ubuntu trusty main universe
#
# The script first checks that it can find /etc/apt/sources.list since this
# is where the repository will need to be added. If the file is not found
# the script will exit with an error
#
# The inner if-blocks add each component of the repository individually (main and universe)
# They first check if any other entries in the sources already include this component, since
# if there are duplicated entries many warnings will show up when running apt-get update
# 
# The regex expressions match lines such as deb http://ca.archive.ubuntu.com/ubuntu trusty main restricted.
# Even if "main" is embedded somewhere in the line it is still detected as already being added
# so the main repository won't be added again
#
sources_file="/etc/apt/sources.list"
if [ -f $sources_file ]; then
    if ! grep -q "^deb.*trusty .*main.*" $sources_file; then
        echo "Adding the trusty main repository..."
        add-apt-repository "deb http://ca.archive.ubuntu.com/ubuntu trusty main"
    fi

    if ! grep -q "^deb.*trusty .*universe.*" $sources_file; then
        echo "Adding the trusty universe repository..."
        add-apt-repository "deb http://ca.archive.ubuntu.com/ubuntu trusty universe"
    fi
else
    echo "Error: Could not find file /etc/apt/sources.list"
    exit
fi

# Update sources
apt-get update

echo Installing packages required to run HOST SOFTWARE...
apt-get install ${host_software_packages[@]} -y --force-yes
echo Done installing packages required to run HOST SOFTWARE

# Copy the thunderbots.rules file to /etc/udev/rules.d to get usb/dongle access
cp $parent_path/99-thunderbots.rules /etc/udev/rules.d
                     
echo Done software setup...

