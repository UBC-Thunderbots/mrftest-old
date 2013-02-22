#ifndef FW_FB_H
#define FW_FB_H

#include "fw/ihex.h"

namespace Firmware {
	void fb_upload(const IntelHex &hex, bool leave_powered);
}

#endif

