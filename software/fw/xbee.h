#ifndef FW_XBEE_H
#define FW_XBEE_H

#include "fw/ihex.h"

namespace Firmware {
	void xbee_upload(const IntelHex &hex, bool fpga, int robot);
}

#endif

