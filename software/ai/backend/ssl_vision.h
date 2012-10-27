#ifndef AI_BACKEND_SSL_VISION_H
#define AI_BACKEND_SSL_VISION_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <glibmm/iochannel.h>
#include <sigc++/signal.h>

class SSL_WrapperPacket;

namespace AI {
	namespace BE {
		class VisionReceiver : public NonCopyable {
			public:
				sigc::signal<void, const SSL_WrapperPacket &> signal_vision_data;

				VisionReceiver(int multicast_interface);
				~VisionReceiver();

			private:
				FileDescriptor sock;
				sigc::connection conn;
				bool receive_packet(Glib::IOCondition);
		};
	}
}

#endif

