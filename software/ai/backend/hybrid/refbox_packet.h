#ifndef AI_BACKEND_HYBRID_REFBOX_PACKET_H
#define AI_BACKEND_HYBRID_REFBOX_PACKET_H

#include "util/bitcodec_primitives.h"

#define BITCODEC_DEF_FILE "ai/backend/hybrid/refbox_packet.def"
#define BITCODEC_STRUCT_NAME RefboxPacket
#define BITCODEC_ANON_NAMESPACE
#define BITCODEC_GEN_HEADER
#define BITCODEC_GEN_SOURCE
#include "util/bitcodec.h"
#undef BITCODEC_GEN_SOURCE
#undef BITCODEC_GEN_HEADER
#undef BITCODEC_ANON_NAMESPACE
#undef BITCODEC_STRUCT_NAME
#undef BITCODEC_DEF_FILE

#endif

