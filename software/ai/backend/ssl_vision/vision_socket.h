#ifndef AI_BACKEND_SSL_VISION_H
#define AI_BACKEND_SSL_VISION_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <glibmm/iochannel.h>
#include <sigc++/signal.h>

class SSL_WrapperPacket;

namespace AI {
	namespace BE {
		namespace SSLVision {
			class VisionSocket : public NonCopyable {
				public:
					sigc::signal<void, const SSL_WrapperPacket &> signal_vision_data;

					VisionSocket(int multicast_interface);
					~VisionSocket();

				private:
					FileDescriptor sock;
					sigc::connection conn;
					bool receive_packet(Glib::IOCondition);
			};
		}
	}
}

#endif

