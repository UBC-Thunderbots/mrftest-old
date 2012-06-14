#ifndef FW_MRF_H
#define FW_MRF_H

#include "fw/ihex.h"

namespace Firmware {
	void mrf_upload(const IntelHex &hex, unsigned int robot);
}

#endif

