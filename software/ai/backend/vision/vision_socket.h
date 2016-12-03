#ifndef AI_BACKEND_SSL_VISION_VISION_SOCKET_H
#define AI_BACKEND_SSL_VISION_VISION_SOCKET_H


#include "util/fd.h"
#include "util/noncopyable.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "ai/common/time.h"
#include <string>
#include <glibmm/iochannel.h>
#include <sigc++/signal.h>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>


class SSL_WrapperPacket;


namespace AI {
	namespace BE {
		namespace Vision {
			class VisionSocket final : public NonCopyable {
				public:
					sigc::signal<void, const SSL_WrapperPacket &> signal_vision_data;


					explicit VisionSocket(int multicast_interface, const std::string &port);
					~VisionSocket();

					std::mutex packets_mutex;
					std::queue<std::pair<SSL_WrapperPacket, AI::Timestamp>> vision_packets;


				private:
					FileDescriptor sock;
					sigc::connection conn;
					std::thread vision_thread;
					void vision_loop();
					bool receive_packet(Glib::IOCondition);
					int counter = 0; //Temporary (for testing)
			};
		}
	}
}


#endif
