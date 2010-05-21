#define DEBUG 0
#include "ai/world/world.h"
#include "geom/angle.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "proto/messages_robocup_ssl_geometry.pb.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "util/dprint.h"
#include "util/sockaddrs.h"
#include <cerrno>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <stdint.h>

namespace {
	/**
	 * The number of vision failures to tolerate before assuming the robot is
	 * gone and removing it from the system. Note that this should be fairly
	 * high because the vision failure count includes instances of a packet
	 * arriving from a camera that cannot see the robot (this is expected to
	 * cause a failure to be counted which will then be zeroed out a moment
	 * later as the camera which can see the robot sends its packet).
	 */
	static const unsigned int MAX_VISION_FAILURES = 15;
}

world::ptr world::create(const config &conf, const std::vector<xbee_drive_bot::ptr> &xbee_bots) {
	ptr p(new world(conf, xbee_bots));
	return p;
}

void world::flip_ends() {
	// Change the flag.
	east_ = !east_;

	// Swap the ball.
	ball_->sign = east_ ? -1 : 1;
	ball_->clear_prediction(-ball_->position(), angle_mod(-ball_->orientation()));

	// Swap the robots.
	static team * const teams[2] = { &friendly, &enemy };
	for (unsigned int i = 0; i < 2; ++i) {
		const team &tm(*teams[i]);
		for (unsigned int j = 0; j < tm.size(); ++j) {
			robot::ptr bot(tm.get_robot(j));
			bot->sign = east_ ? -1 : 1;
			bot->clear_prediction(-bot->position(), angle_mod(bot->orientation() + M_PI));
		}
	}

	// Fire the signal.
	signal_flipped_ends.emit();
}

void world::flip_refbox_colour() {
	// Change the flag.
	refbox_yellow_ = !refbox_yellow_;

	// Swap the teams' scores.
	unsigned int temp = friendly.score();
	friendly.score(enemy.score());
	enemy.score(temp);

	// Invert the play type.
	playtype(playtype::invert[playtype()]);

	// Fire the signal.
	signal_flipped_refbox_colour.emit();
}

world::world(const config &conf, const std::vector<xbee_drive_bot::ptr> &xbee_bots) : conf(conf), east_(false), refbox_yellow_(false), vision_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP), refbox_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP), ball_(ball::create()), xbee_bots(xbee_bots), playtype_(playtype::halt), vis_view(this) {
	vision_socket.set_blocking(false);
	const int one = 1;
	if (setsockopt(vision_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		throw std::runtime_error("Cannot set SO_REUSEADDR.");
	}
	sockaddrs sa;
	sa.in.sin_family = AF_INET;
	sa.in.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.in.sin_port = htons(10002);
	std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
	if (bind(vision_socket, &sa.sa, sizeof(sa.in)) < 0) {
		throw std::runtime_error("Cannot bind to port 10002 for vision data.");
	}
	ip_mreqn mcreq;
	mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.2");
	mcreq.imr_address.s_addr = htonl(INADDR_ANY);
	mcreq.imr_ifindex = 0;
	if (setsockopt(vision_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
		LOG("Cannot join multicast group 224.5.23.2 for vision data.");
	}
	Glib::signal_io().connect(sigc::mem_fun(this, &world::on_vision_readable), vision_socket, Glib::IO_IN);

	refbox_socket.set_blocking(false);
	if (setsockopt(refbox_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		throw std::runtime_error("Cannot set SO_REUSEADDR.");
	}
	sa.in.sin_port = htons(10001);
	if (bind(refbox_socket, &sa.sa, sizeof(sa.in)) < 0) {
		throw std::runtime_error("Cannot bind to port 10001 for refbox data.");
	}
	mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.1");
	if (setsockopt(refbox_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
		LOG("Cannot join multicast group 224.5.23.1 for refbox data.");
	}
	Glib::signal_io().connect(sigc::mem_fun(this, &world::on_refbox_readable), refbox_socket, Glib::IO_IN);
}

bool world::on_vision_readable(Glib::IOCondition) {
	DPRINT("Enter on_vision_readable.");

	// Receive a packet.
	uint8_t buffer[65536];
	ssize_t len = recv(vision_socket, buffer, sizeof(buffer), 0);
	if (len < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			LOG("Cannot receive packet from SSL-Vision.");
		}
		DPRINT("Exit on_vision_readable (1).");
		return true;
	}

	// Decode it.
	SSL_WrapperPacket packet;
	if (!packet.ParseFromArray(buffer, len)) {
		LOG("Received malformed SSL-Vision packet.");
		DPRINT("Exit on_vision_readable (2).");
		return true;
	}

	// If it contains geometry data, update the field shape.
	if (packet.has_geometry()) {
		const SSL_GeometryData &geom(packet.geometry());
		const SSL_GeometryFieldSize &fsize(geom.field());
		field_.update(fsize);
		signal_geometry.emit();
	}

	// If it contains ball and robot data, update the ball and the teams.
	if (packet.has_detection()) {
		const SSL_DetectionFrame &det(packet.detection());

		// Check for a sensible camera ID number.
		if (det.camera_id() >= 2) {
			LOG(Glib::ustring::compose("Received SSL-Vision packet for unknown camera %1.", det.camera_id()));
			DPRINT("Exit on_vision_readable (3).");
			return true;
		}

		// Keep a local copy of all detection frames.
		detections[det.camera_id()].CopyFrom(det);

		// Update the ball.
		{
#warning use the ball filter here
			// Find the detection with highest confidence across all cameras
			// that have sent us data.
			unsigned int best_detection = -1;
			int best_index = -1;
			double best_confidence = 0.0;
			for (unsigned int i = 0; i < 2; ++i) {
				if (detections[i].IsInitialized()) {
					for (int j = 0; j < detections[i].balls_size(); ++j) {
						const SSL_DetectionBall &b(detections[i].balls(j));
						if (b.confidence() > best_confidence) {
							best_detection = i;
							best_index = j;
							best_confidence = b.confidence();
						}
					}
				}
			}

			// Update the ball IF the best position is within the packet we just
			// received (there's no reason to do anything otherwise, since it
			// would only result in polluting the least-squares regression
			// database with old data that is falsely claiming to be new).
			if (best_detection == det.camera_id()) {
				ball_->update(det.balls(best_index));
			}
		}

		// Update the robots.
		{
			std::vector<bool> used_data[2];
			static team * const teams[2] = { &friendly, &enemy };
			static const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &(SSL_DetectionFrame::*const colour_fns[2])() const = { &SSL_DetectionFrame::robots_yellow, &SSL_DetectionFrame::robots_blue };
			static const bool colours[2] = { true, false };
			for (unsigned int j = 0; j < 2; ++j) {
				const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep((det.*colour_fns[j])());
				const bool colour(colours[j]);
				used_data[j].resize(rep.size(), false);
				for (int k = 0; k < rep.size(); ++k) {
					const SSL_DetectionRobot &detbot(rep.Get(k));
					if (detbot.has_robot_id()) {
						const unsigned int pattern_index = detbot.robot_id();
						for (unsigned int m = 0; m < 2; ++m) {
							team &tm(*teams[m]);
							for (unsigned int n = 0; n < tm.size(); ++n) {
								robot::ptr bot(tm.get_robot(n));
								if (bot->yellow == colour && bot->pattern_index == pattern_index && !bot->seen_this_frame) {
									bot->seen_this_frame = true;
									bot->update(detbot);
									used_data[j][k] = true;
								}
							}
						}
					}
				}
			}

			// Count failures.
			for (unsigned int i = 0; i < 2; ++i) {
				team &tm(*teams[i]);
				for (unsigned int j = 0; j < tm.size(); ++j) {
					robot::ptr bot(tm.get_robot(j));
					if (!bot->seen_this_frame) {
						++bot->vision_failures;
					} else {
						bot->vision_failures = 0;
					}
					bot->seen_this_frame = false;
					if (bot->vision_failures >= MAX_VISION_FAILURES) {
						tm.remove(j);
						--j;
					}
				}
			}

			// Look for new robots and create them.
			for (unsigned int j = 0; j < 2; ++j) {
				const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep((det.*colour_fns[j])());
				const bool colour(colours[j]);
				for (int k = 0; k < rep.size(); ++k) {
					if (!used_data[j][k]) {
						const SSL_DetectionRobot &detbot(rep.Get(k));
						if (detbot.has_robot_id()) {
							const unsigned int pattern_index = detbot.robot_id();
							xbee_drive_bot::ptr xbeebot;
							for (unsigned int m = 0; m < conf.robots().size(); ++m) {
								if (conf.robots()[m].yellow == colour && conf.robots()[m].pattern_index == pattern_index) {
									xbeebot = xbee_bots[m];
								}
							}
							if (xbeebot) {
								player::ptr plr(player::create(colour, pattern_index, xbeebot));
								plr->sign = east_ ? -1 : 1;
								plr->update(detbot);
								friendly.add(plr);
							} else {
								robot::ptr bot(robot::create(colour, pattern_index));
								bot->sign = east_ ? -1 : 1;
								bot->update(detbot);
								enemy.add(bot);
							}
						}
					}
				}
			}
		}
	}

	// Notify any attached visualizers. Because visualizers call position() and
	// orientation() on robots and the ball, and because those objects are
	// predictable and hence implement those functions as estimates based on a
	// delta time, we need to lock the prediction timestamps of those objects
	// before handing them to the visualizer for rendering, otherwise we won't
	// actually see them move!
	{
		static team * const teams[2] = { &friendly, &enemy };
		for (unsigned int i = 0; i < 2; ++i) {
			const team &tm(*teams[i]);
			for (unsigned int j = 0; j < tm.size(); ++j) {
				const robot::ptr bot(tm.get_robot(j));
				bot->lock_time();
			}
		}
		ball_->lock_time();
		DPRINT("Updating visualizers.");
		vis_view.signal_visdata_changed.emit();
	}

	DPRINT("Exit on_vision_readable (4).");
	return true;
}

bool world::on_refbox_readable(Glib::IOCondition) {
	DPRINT("Enter on_refbox_readable.");

	// Receive the packet.
	struct __attribute__((packed)) {
		char cmd;
		uint8_t counter;
		uint8_t goals_blue;
		uint8_t goals_yellow;
		uint16_t time_remaining;
	} packet;
	ssize_t len = recv(refbox_socket, &packet, sizeof(packet), 0);

	// Check for valid length.
	if (len < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			LOG("Cannot receive packet from refbox.");
		}
		return true;
	} else if (len != sizeof(packet)) {
		LOG("Received martian packet from refbox.");
		DPRINT("Exit on_refbox_readable (1).");
		return true;
	}

	// Install the teams's scores.
	if (refbox_yellow_) {
		friendly.score(packet.goals_yellow);
		enemy.score(packet.goals_blue);
	} else {
		friendly.score(packet.goals_blue);
		enemy.score(packet.goals_yellow);
	}

	// Install the play type.
	{
		// If we don't recognize the command, we will leave the current play
		// type in force. We need to translate the current play type back to
		// blue's point of view.
		playtype::playtype pt = playtype_;
		if (refbox_yellow_) {
			pt = playtype::invert[pt];
		}

		// Compute the play type from blue's point of view based on the incoming
		// command.
		switch (packet.cmd) {
			case 'H': // HALT
				pt = playtype::halt;
				break;

			case 'S': // STOP
			case 'z': // END TIMEOUT
				pt = playtype::stop;
				break;

			case ' ': // NORMAL START
#warning this is horrible, it needs to be rewritten with something useful
#warning right now it latches into the "execute" state for exactly one packet before changing to "play"
#warning work with AI people to figure out a better division between playtypes and something else that better describes the semantics?
				if (playtype_ == playtype::prepare_kickoff_friendly) {
					pt = playtype::execute_kickoff_friendly;
				} else if (playtype_ == playtype::prepare_kickoff_enemy) {
					pt = playtype::execute_kickoff_enemy;
				} else if (playtype_ == playtype::prepare_penalty_friendly) {
					pt = playtype::execute_penalty_friendly;
				} else if (playtype_ == playtype::prepare_penalty_enemy) {
					pt = playtype::execute_penalty_enemy;
				} else {
					pt = playtype::play;
				}
				break;

			case 's': // FORCE START
				pt = playtype::play;
				break;

			case 'k': // KICKOFF YELLOW
				pt = playtype::prepare_kickoff_enemy;
				break;

			case 'K': // KICKOFF BLUE
				pt = playtype::prepare_kickoff_friendly;
				break;

			case 'p': // PENALTY YELLOW
				pt = playtype::prepare_penalty_enemy;
				break;

			case 'P': // PENALTY BLUE
				pt = playtype::prepare_penalty_friendly;
				break;

			case 'f': // DIRECT FREE KICK YELLOW
				pt = playtype::execute_direct_free_kick_enemy;
				break;

			case 'F': // DIRECT FREE KICK BLUE
				pt = playtype::execute_direct_free_kick_friendly;
				break;

			case 'i': // INDIRECT FREE KICK YELLOW
				pt = playtype::execute_indirect_free_kick_enemy;
				break;

			case 'I': // INDIRECT FREE KICK BLUE
				pt = playtype::execute_indirect_free_kick_friendly;
				break;

			case 'h': // HALF TIME
			case 't': // TIMEOUT YELLOW
			case 'T': // TIMEOUT BLUE
				pt = playtype::pit_stop;
				break;

			case '1': // BEGIN FIRST HALF
			case '2': // BEGIN SECOND HALF
			case 'o': // BEGIN OVERTIME 1
			case 'O': // BEGIN OVERTIME 2
			case 'a': // BEGIN PENALTY SHOOTOUT
			case 'g': // GOAL YELLOW
			case 'G': // GOAL BLUE
			case 'd': // REVOKE GOAL YELLOW
			case 'D': // REVOKE GOAL BLUE
			case 'y': // YELLOW CARD YELLOW
			case 'Y': // YELLOW CARD BLUE
			case 'r': // RED CARD YELLOW
			case 'R': // RED CARD BLUE
			case 'c': // CANCEL
				break;
		}

		// If we are yellow, invert the command.
		if (refbox_yellow_) {
			pt = playtype::invert[pt];
		}

		// Lock in the new play type.
		playtype(pt);
	}

	DPRINT("Exit on_refbox_readable (2).");
	return true;
}

void world::playtype(playtype::playtype pt) {
	if (pt != playtype_) {
		DPRINT(Glib::ustring::compose("Changing play type to %1.", playtype::descriptions_generic[pt]));
		playtype_ = pt;
		signal_playtype_changed.emit();
	}
}

