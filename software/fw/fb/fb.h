#ifndef FW_FB_H
#define FW_FB_H

#include "fw/ihex.h"
#include <cstdint>

namespace Firmware {
	void fb_upload(const IntelHex &hex, bool onboard, bool leave_powered, uint16_t start_page);
}

#endif

