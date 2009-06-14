#!/bin/bash
CUR_VERSION=$(svn info . | fgrep 'Revision: ' | sed 's/Revision: //')
echo "#define FIRMWARE_REVISION ${CUR_VERSION}" > version.h

