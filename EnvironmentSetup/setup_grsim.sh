#!/bin/bash
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# UBC Thunderbots grSim Setup
#
# This script must be run with sudo! root permissions are required to install
# packages and copy files to the rules.d directory. The reason that the script
# must be run with sudo rather than the individual commands using sudo, is that
# when running CI within Docker, the sudo command does not exist since
# everything is automatically run as root.
#
# This script will install all the required libraries and dependencies to run grSim
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#

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

# Install packages required to run grSim
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

apt-get update

echo Installing packages required to run grSim...
apt-get install ${host_software_packages[@]} -y
echo Done installing packages required to run grSim

cd $parent_path     # cd back to parent so commands are relative to the script location

# Clone and install vartypes.
# These are required to run grSim
# This assumes you already have the correct robocup folder structure
# The vartypes folder will be placed in robocup->vartypes
# If a vartypes folder already exists this step will be skipped
vartypes_location="../../../"
vartypes_path="$vartypes_location/vartypes"
if [ -d $vartypes_path ]; then
    echo "Removing old vartypes..."
    rm -r $vartypes_path
fi

echo "Installing vartypes..."
cd $vartypes_location
git clone https://github.com/roboime/vartypes
cd vartypes
make
make install
echo "Done..."

cd $parent_path     # cd back to parent so commands are relative to the script location

# Clone and install grSim.
# This assumes you already have the correct robocup folder structure
# The vartypes folder will be placed in robocup->vartypes
# If a grSim folder already exists this step will be skipped
grSim_location="../../../"
grSim_path="$grSim_location/grSim"
if [ -d $grSim_path ]; then
    echo "Removing old grSim..."
    rm -r $grSim_path
fi

echo "Installing grSim..."
cd $grSim_location
git clone https://github.com/roboime/grSim
cd grSim
mkdir build
cd build
cmake ..
make
echo "Done..."

if ! grep -q "^/usr/local/lib" "/etc/ld.so.conf"; then
    echo "Appending /usr/local/lib to /etc/ld.so.conf..."
    echo "/usr/local/lib"$'\n' | tee -a "/etc/ld.so.conf" # The $'\n' adds a newline
    ldconfig
    echo "Your /etc/ld.so.conf file has been modified. Please log out and log
    back in for these changes to take effect"
fi

echo "Done installing grSim..."

