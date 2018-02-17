#!/bin/bash

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# UBC Thunderbots Linux grSim Setup
#
# This script must be run with sudo! root permissions are required to install
# packages and modify the /etc/ld.so.conf file. The reason that the script
# must be run with sudo rather than the individual commands using sudo, is that
# when running CI within Docker, the sudo command does not exist since
# everything is automatically run as root.
#
# This script will install all the required libraries and dependencies to run
# grSim. This only includes the simulator (grSim), and not anything related to
# our AI. See the setup_software.sh script for that.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#


# Preliminary checks
echo "================================================================"
echo "Performing preliminary checks."
echo "================================================================"

# First, we test whether bash supports arrays.
# From http://tldp.org/LDP/Bash-Beginners-Guide/html/sect_10_02.html
testarray[0]='test' || (echo 'Failure: arrays not supported in this version of bash.' && exit 2)

# Make sure that we are running as root
if [ "$EUID" -ne 0 ]
  then echo "Please run as root via the \`sudo\` command"
  exit 2
fi

# Save the parent dir of this script since we want to return here
# so that commands are run relative to the location of this script
parent_path=$(dirname -- "$(readlink -f -- "$BASH_SOURCE")")
cd $parent_path


# Install required packages
echo "================================================================"
echo "Downloading and installing required packages."
echo "================================================================"

# The list of packages required to build and run grSim
# See the github repo for the list of dependencies
# https://github.com/RoboCup-SSL/grSim
host_software_packages=(
    libqt4-dev
    libqt4-opengl-dev
    libgl1-mesa-dev
    libglu1-mesa-dev
    protobuf-compiler
    mercurial
    git
    cmake
)

# Update the list of sources
apt-get update

# Install the packages
apt-get install ${host_software_packages[@]} -y


# Install vartypes. These are required by grSim
echo "================================================================"
echo "Installing Vartypes"
echo "================================================================"

# Clone and install vartypes.
# These are required to run grSim
# This assumes you already have the correct robocup folder structure
# The vartypes folder will be placed in robocup/vartypes
# If a vartypes folder already exists it will be removed first
vartypes_location="../../../"
vartypes_path="$vartypes_location/vartypes"

if [ -d $vartypes_path ]; then
    echo "Removing old vartypes..."
    rm -r $vartypes_path
fi

# Change to the parent directory of the vartypes_path (the vartypes_location)
# and perform the clone here so we end up with only 1 vartypes subfolder
# (no nested vartypes folders)
cd $vartypes_location
git clone https://github.com/roboime/vartypes
cd vartypes
make
make install

# Return to the parent directory of the script so commands are relative to here
cd $parent_path


# Doing this helps grSim find libraries, but requires the user to log out and
# log back in. If grSIm fails to build due to not being able to find libraries,
# the user should log out, log in, and then try again.
echo "================================================================"
echo "Helping grSim find libraries"
echo "================================================================"

# We need to add '/usr/local/lib' the the /etc/ld.so.conf file, which helps
# grSim find library files (This normally doesn't seem to be an issue, but we
# are keeping the instructions here for legacy purposes. If in the future this
# is confirmed to no longer be necessary, it can be removed).
#
# We first check that the text doesn't already exist in the file, so we don't
# add it more than once
if ! grep -q "^/usr/local/lib" "/etc/ld.so.conf"; then
    echo "/usr/local/lib"$'\n' | tee -a "/etc/ld.so.conf" # The $'\n' adds a newline
    ldconfig
fi


# Install grSim
echo "================================================================"
echo "Installing grSim"
echo "================================================================"

# Clone and install grSim.
# This assumes you already have the correct robocup folder structure
# The grSim folder will be placed in robocup/grSim
grSim_location="../../../"
grSim_path="$grSim_location/grSim"
if [ -d $grSim_path ]; then
    echo "Removing old grSim..."
    rm -r $grSim_path
fi

cd $grSim_location
git clone https://github.com/roboime/grSim
cd grSim

# This git command is here because grSim was updated to use a new protobuf
# protocol that our AI does not support yet. Until our AI is updated, we need to
# revert to a grSim version before this protobuf change was made and build that
# version instead
git reset --hard 68b322d085a84690b965815b1e035a908ebc75ee
if [ $? -eq 0 ]; then
    echo grSim reverted to working version
else
    echo grSim revert failed
fi

mkdir build
cd build
cmake ..
make


# Report done
echo "================================================================"
echo "Done grSim Setup"
echo ""
echo "If grSim failed to build because it couldn't find certain libraries,\
log out and log back in so the library changes take effect, then run the script
again. Otherwise you can disregard this message."
echo "================================================================"

