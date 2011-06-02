#ifndef XBEE_TESTMODEPACKET_H
#define XBEE_TESTMODEPACKET_H

#include "util/bitcodec_primitives.h"

#define BITCODEC_DEF_FILE "xbee/testmodepacket.def"
#define BITCODEC_NAMESPACE XBeePackets
#define BITCODEC_STRUCT_NAME TestMode
#define BITCODEC_GEN_HEADER
#include "util/bitcodec.h"
#undef BITCODEC_GEN_HEADER
#undef BITCODEC_STRUCT_NAME
#undef BITCODEC_NAMESPACE
#undef BITCODEC_DEF_FILE

#endif

