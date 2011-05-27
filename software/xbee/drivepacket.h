#ifndef XBEE_DRIVEPACKET_H
#define XBEE_DRIVEPACKET_H

#include "util/bitcodec_primitives.h"

#define BITCODEC_DEF_FILE "xbee/drivepacket.def"
#define BITCODEC_NAMESPACE XBeePackets
#define BITCODEC_STRUCT_NAME Drive
#include "util/bitcodec.h"
#undef BITCODEC_STRUCT_NAME
#undef BITCODEC_NAMESPACE
#undef BITCODEC_DEF_FILE

#endif

