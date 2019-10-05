#!/bin/bash

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# UBC Thunderbots Linux Software Setup
#
# This script must be run with sudo! root permissions are required to install
# packages and copy files to the /etc/udev/rules.d directory. The reason that the script
# must be run with sudo rather than the individual commands using sudo, is that
# when running CI within Docker, the sudo command does not exist since
# everything is automatically run as root.
#
# This script will install all the required libraries and dependencies to build
# and run the Thunderbots codebase. This includes being able to run the ai and
# unit tests
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
    gtkmm3            # gtkmm-3.0, used to create the gui?
    libxml++2.6       # libxml++
    gsl               # libgsl0-dev
    libusb            # libusb-1.0.0-dev
    lbzip2            # bzip2, used for archiving and storing our log files
    cmake             # Required to build the Google Test libraries
)

# Install the packages
pacman -S ${host_software_packages[@]}

# Set up MRF/USB permissions. This allows the user to run the ai (with the
# dongle and radio) and flash the robots without requiring `sudo` permissions
echo "================================================================"
echo "Setting up MRF/USB permissions"
echo "================================================================"
# Create a 'thunderbots' group and add the user to it. This group is used by our
# 99-thunderbots.rules file to set USB permissions
groupadd thunderbots

# The SUDO_USER environment variable gives the original user of the command
# even if it's run with sudo (otherwise, commands like whoami return `root`,
# which we don't want since we want to add the normal user to the thunderbots
# group). Since we expect this script to be run with sudo, we can use this
# evironment variable to get the username of the user that ran this script.
# If for whatever reason the SUDO_USER variable returns an empty, this means we
# are likely running in CI. Warn the user just in case, and do not try add the
# user.
#
# See https://stackoverflow.com/questions/4598001/how-do-you-find-the-original-user-through-multiple-sudo-and-su-commands
# for more information on these environment variables and how they behave
user=`echo "$SUDO_USER"`
if [ -z "$user" ]; then
    echo "================================================================"
    echo "Warning: The SUDO_USER environment variable was null or empty. Make\
sure you are running this script with the `sudo` command,
Eg. 'sudo ./setup_software.sh'. If this is running in CI, disregard this error."
    echo "================================================================"
else
    # Add the user to the thunderbots group, and copy our rules file.
    # The user will need to log out and log back in after this for the permissions
    # changes to take effect
    gpasswd -a "$user" thunderbots
    cp $parent_path/99-thunderbots.rules /etc/udev/rules.d
fi


# Report done
echo "================================================================"
echo "Done Software Setup"
echo ""
echo "Make sure to log out and log back in for permissions changes to take effect!"
echo "================================================================"

