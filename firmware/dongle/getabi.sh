#!/bin/bash
if which arm-elf-gcc &>/dev/null; then
	echo arm-elf
	exit 0
elif which arm-linux-gnueabi-gcc &>/dev/null; then
	echo arm-elf-linux-gnueabi
	exit 0
else
	echo "No ARM toolchain found" >&2
	exit 1
fi
