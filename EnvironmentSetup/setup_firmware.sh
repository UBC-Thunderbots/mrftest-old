#!/bin/bash

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# UBC Thunderbots Linux Firmware Setup
#
# This script must be run with sudo! root permissions are required to install
# packages and copy files to the /etc/udev/rules.d directory. The reason that the script
# must be run with sudo rather than the individual commands using sudo, is that
# when running CI within Docker, the sudo command does not exist since
# everything is automatically run as root.
#
# This script will install all the required libraries and dependencies in order
# to build and upload the Thunderbots firmware. This includes the firmware for
# the robots as well as the radio dongle.
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

# The list of packages required to build and run our software
host_software_packages=(
    libgmp-dev
    libmpc-dev
    libmpfr-dev
    libncursesw5-dev
    texinfo
)

# The dependencies for these packages will be installed so we can build our own
# versions. This is required for building the arm-toolchain
build_dep_packages=(
    binutils
    gcc
    gdb
)

# Update sources before we try install software-properties-common
apt-get update

# Install `software-properties-common` that provides the `add-apt-repository`
# command used later on in this script
# Although the -y and --force-yes seems redundant, CI does not work otherwise
# and some people have reported that only -y is not always effective
apt-get install software-properties-common -y --force-yes

# This if-block will add the required repositories to install the build-deps.
# The full entry that must be added to the list of sources is
# deb-src http://ca.archive.ubuntu.com/ubuntu/ xenial main restricted
#
# The version (Xenial) may be updated as new ubuntu versions are released. We
# required the deb-src rather than the deb because we need the sources for
# various packages. This entry is not enabled by default, so we need to add it.
#
# The script first checks that it can find /etc/apt/sources.list since this
# is where the repository will need to be added. If the file is not found
# the script will exit with an error.
#
# The inner if-blocks add each component of the repository individually (main
# and restricted)
# They first check if any other entries in the sources already include this component, since
# if there are duplicated entries many warnings will show up when running apt-get update
# 
# The regex expressions match lines such as deb-src
# http://ca.archive.ubuntu.com/ubuntu xenial main restricted.
# Even if "main" is embedded somewhere in the line it is still detected as already being added
# so the main repository won't be added again, If the repository is not found in
# the file already, it is added
#
sources_file="/etc/apt/sources.list"
if [ -f $sources_file ]; then
    if ! grep -q "^deb-src.*xenial .*main.*" $sources_file; then
        echo "Adding the xenial main src repository..."
        add-apt-repository "deb-src http://ca.archive.ubuntu.com/ubuntu/ xenial main"
    fi

    if ! grep -q "^deb-src.*xenial .*restricted.*" $sources_file; then
        echo "Adding the xenial restricted src repository..."
        add-apt-repository "deb-src http://ca.archive.ubuntu.com/ubuntu/ xenial restricted"
    fi
else
    echo "Error: Could not find file /etc/apt/sources.list"
    exit
fi

# Update sources after adding repositories
apt-get update

# Install the packages
apt-get build-dep ${build_dep_packages[@]} -y --force-yes
apt-get install ${host_software_packages[@]} -y --force-yes

# Create a temporary directory
# Putting quotes around these paths causes them to be created relative to the
# script, using the '~' character literally rather than expanding it to the home
# directory
mkdir -p ~/tbots-toolchains-temp mathew
cd ~/tbots-toolchains-temp

# Run the build-arm-toolchain script. We assume it is in the same folder and
# path as this script (setup firmware) in the EnvironmentSetup folder, which is
# the parent_path variable
$parent_path/build-arm-toolchain

# Add a line in the .bashrc file to add the tbots-toolchain to the PATH. This
# line will only b added if it does not already exist
bashrc_path="$HOME/.bashrc"
if [ -a $bashrc_path ]; then
    if ! grep -q 'export PATH="$PATH:$HOME/tbots-toolchains/bin"' $bashrc_path; then
        echo "Adding toolchains folder to bashrc"
        # Use the $HOME environment variable rather than the ~ abbreviateion for
        # home, since the ~ gets added literally and causes the `make` command
        # to be unable to find the binaries (despite them showing up in your
        # PATH)
        echo 'export PATH="$PATH:$HOME/tbots-toolchains/bin"' | tee -a $bashrc_path
    fi
else
    echo "Error: Could not find $bashrc_path file"
    exit
fi

# Check dfu-util version. Must be >= 0.7
# Version comparison function is from:
# http://ask.xmodulo.com/compare-two-version-numbers.html<Paste>
echo "================================================================"
echo "Checking dfu-util version"
echo "================================================================"
function version_ge() { test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" == "$1"; }

dfu_version=`dfu-util --version | grep -E "dfu-util [0-9|\.]+" | cut -f2 -d' '`
if version_ge "$dfu_version" "0.7" ; then
   echo "Your dfu-util version is >= 0.7"
else
	echo "================================================================"
	echo "Error: Your dfu-util version is less than 0.7. Please manually\
		  install a version of dfu-util greater than or equal to 0.7"
	echo "Your version is $dfu_version"
	echo "================================================================"	
fi

# Report done
echo "================================================================"
echo "Done Firmware Setup"
echo "================================================================"

