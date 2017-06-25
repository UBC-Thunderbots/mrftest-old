#ifndef AI_BACKEND_SSL_VISION_VISION_THREAD_H
#define AI_BACKEND_SSL_VISION_VISION_THREAD_H


#include "util/fd.h"
#include "util/noncopyable.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "ai/common/time.h"
#include "ai/backend/backend.h"
#include "geom/point.h"
#include "mrf/dongle.h"
#include <string>
#include <glibmm/iochannel.h>
#include <sigc++/signal.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <tuple>




namespace AI {
	namespace BE {
		namespace Vision {
			class VisionInfo final : public NonCopyable {
				public:
					bool defendingEast();
					bool friendlyIsYellow();
					Point ballPos();
					bool dataValid();
						
					void setDefendingEast(bool);
					void setFriendlyIsYellow(bool);
					void setBallPos(Point);
					void setDataValid(bool);
				private:
					std::atomic<bool> data_valid;
					std::atomic<bool> defending_east;
					std::atomic<bool> friendly_yellow;
					Point ball_pos;
					std::mutex ball_mtx;
			};
			/* This class is responsible for the following things:
				-spinning up a thread to monitor and handle incoming packets
				-destroying above thread in the destructor
				-proccessing incoming packets for robot positions
				-maintaining an object with knowledge of:
					-which side of the field we are defending
					-what colour we are
					-ball position
					-ignored cameras
				-have access to the dongle
				-writing vision packets to a queue to be processed further by the main thread
				-transmitting robot positions over the dongle
			*/
			class VisionThread final : public NonCopyable {
				public:
					VisionInfo vis_inf;
					static constexpr std::size_t MAX_PATTERN = 8; // camera packet structured for robot nums < 8
					VisionThread(MRFDongle &dongle_, int multicast_interface, const std::string &port, const std::vector<bool> &disable_cameras);
					~VisionThread();
					void update_info(AI::BE::Backend::FieldEnd friendly_side, AI::Common::Colour friendly_colour, Point ball_pos); 		
					std::queue<SSL_WrapperPacket> vision_packets;
					std::mutex packets_mutex;
				private:
					std::atomic<bool> stop_thread;
					FileDescriptor sock;	
					MRFDongle& dongle;
					std::vector<bool> disable_cameras;
					std::thread vision_thread;
					void vision_loop(int, std::string);
					void sock_init(FileDescriptor&, int, std::string);
					void transmit_bots(SSL_WrapperPacket packet);
			};
		}
	}
}


#endif
