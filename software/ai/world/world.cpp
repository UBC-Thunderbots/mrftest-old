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
	 * The number of metres the ball must move from a kickoff or similar
	 * playtype until we consider that the ball has been moved and is free to be
	 * approached by either team.
	 */
	static const double BALL_FREE_DISTANCE = 0.09;

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
	// Update the flag.
	refbox_yellow_ = !refbox_yellow_;

	// Flip the current play type, so that the updater will flip it back and
	// have the proper "old" value.
	playtype_ = playtype::invert[playtype_];

	// Now run the updater.
	update_playtype();
}

world::world(const config &conf, const std::vector<xbee_drive_bot::ptr> &xbee_bots) : conf(conf), east_(false), refbox_yellow_(false), vision_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP), ball_(ball::create()), xbee_bots(xbee_bots), playtype_(playtype::halt), playtype_override(playtype::halt), playtype_override_active(false), vis_view(this), ball_filter_(0) {
	vision_socket.set_blocking(false);
	const int one = 1;
	if (setsockopt(vision_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		throw std::runtime_error("Cannot set SO_REUSEADDR.");
	}
	sockaddrs sa;
	sa.in.sin_family = AF_INET;
	sa.in.sin_addr.s_addr = get_inaddr_any();
	sa.in.sin_port = htons(10002);
	std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
	if (bind(vision_socket, &sa.sa, sizeof(sa.in)) < 0) {
		throw std::runtime_error("Cannot bind to port 10002 for vision data.");
	}
	ip_mreqn mcreq;
	mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.2");
	mcreq.imr_address.s_addr = get_inaddr_any();
	mcreq.imr_ifindex = 0;
	if (setsockopt(vision_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
		LOG("Cannot join multicast group 224.5.23.2 for vision data.");
	}
	Glib::signal_io().connect(sigc::mem_fun(this, &world::on_vision_readable), vision_socket, Glib::IO_IN);

	refbox_.signal_command_changed.connect(sigc::mem_fun(this, &world::update_playtype));
}

bool world::on_vision_readable(Glib::IOCondition) {
	// Receive a packet.
	uint8_t buffer[65536];
	ssize_t len = recv(vision_socket, buffer, sizeof(buffer), 0);
	if (len < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			LOG("Cannot receive packet from SSL-Vision.");
		}
		return true;
	}

	// Decode it.
	SSL_WrapperPacket packet;
	if (!packet.ParseFromArray(buffer, len)) {
		LOG("Received malformed SSL-Vision packet.");
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
			return true;
		}

		// Keep a local copy of all detection frames.
		detections[det.camera_id()].CopyFrom(det);

		// Update the ball.
		{
			// Build a vector of all detections so far.
			std::vector<std::pair<point, double> > balls;
			for (unsigned int i = 0; i < 2; ++i) {
				for (int j = 0; j < detections[i].balls_size(); ++j) {
					const SSL_DetectionBall &b(detections[i].balls(j));
					balls.push_back(std::make_pair(point(b.x() / 1000.0, b.y() / 1000.0), b.confidence()));
				}
			}

			// Execute the current ball filter.
			point pos;
			if (ball_filter_) {
				pos = ball_filter_->filter(balls, friendly, enemy);
			}

			// Use the result.
			ball_->update(pos);
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
								if (bot->yellow == colour && bot->pattern_index == pattern_index) {
									if (!bot->seen_this_frame) {
										bot->seen_this_frame = true;
										bot->update(detbot);
									}
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
						player::ptr plr(player::ptr::cast_dynamic(bot));
						if (plr) {
							plr->controller.reset();
						}
						bot.reset();
						plr.reset();
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
							Glib::ustring name;
							for (unsigned int m = 0; m < conf.robots().size(); ++m) {
								if (conf.robots()[m].yellow == colour && conf.robots()[m].pattern_index == pattern_index) {
									xbeebot = xbee_bots[m];
									name = conf.robots()[m].name;
								}
							}
							if (xbeebot) {
								player::ptr plr(player::create(name, colour, pattern_index, xbeebot));
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
		vis_view.signal_visdata_changed.emit();
	}

	// Movement of the ball may, potentially, result in a play type change.
	update_playtype();

	return true;
}

void world::override_playtype(playtype::playtype pt) {
	if (pt != playtype_override || !playtype_override_active) {
		DPRINT(Glib::ustring::compose("Setting override play type to %1.", playtype::descriptions_generic[pt]));
		playtype_override = pt;
		playtype_override_active = true;
		update_playtype();
	}
}

void world::clear_playtype_override() {
	if (playtype_override_active) {
		DPRINT("Clearing override play type.");
		playtype_override_active = false;
		update_playtype();
	}
}

void world::update_playtype() {
	playtype::playtype old_pt = playtype_;
	if (refbox_yellow_) {
		old_pt = playtype::invert[old_pt];
	}
	playtype::playtype new_pt = compute_playtype(old_pt);
	if (refbox_yellow_) {
		new_pt = playtype::invert[new_pt];
	}
	if (new_pt != playtype_) {
		playtype_ = new_pt;
		signal_playtype_changed.emit();
	}
}

playtype::playtype world::compute_playtype(playtype::playtype old_pt) {
	if (playtype_override_active) {
		return playtype_override;
	}

	switch (refbox_.command()) {
		case 'H': // HALT
			return playtype::halt;

		case 'S': // STOP
		case 'z': // END TIMEOUT
			return playtype::stop;

		case ' ': // NORMAL START
			switch (old_pt) {
				case playtype::prepare_kickoff_friendly:
					playtype_arm_ball_position = ball_->position();
					return playtype::execute_kickoff_friendly;

				case playtype::prepare_kickoff_enemy:
					playtype_arm_ball_position = ball_->position();
					return playtype::execute_kickoff_enemy;

				case playtype::prepare_penalty_friendly:
					playtype_arm_ball_position = ball_->position();
					return playtype::execute_penalty_friendly;

				case playtype::prepare_penalty_enemy:
					playtype_arm_ball_position = ball_->position();
					return playtype::execute_penalty_enemy;

				case playtype::execute_kickoff_friendly:
				case playtype::execute_kickoff_enemy:
				case playtype::execute_penalty_friendly:
				case playtype::execute_penalty_enemy:
					if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
						return playtype::play;
					} else {
						return old_pt;
					}

				default:
					return playtype::play;
			}

		case 'f': // DIRECT FREE KICK YELLOW
			if (old_pt == playtype::play) {
				return playtype::play;
			} else if (old_pt == playtype::execute_direct_free_kick_enemy) {
				if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return playtype::play;
				} else {
					return playtype::execute_direct_free_kick_enemy;
				}
			} else {
				playtype_arm_ball_position = ball_->position();
				return playtype::execute_direct_free_kick_enemy;
			}

		case 'F': // DIRECT FREE KICK BLUE
			if (old_pt == playtype::play) {
				return playtype::play;
			} else if (old_pt == playtype::execute_direct_free_kick_friendly) {
				if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return playtype::play;
				} else {
					return playtype::execute_direct_free_kick_friendly;
				}
			} else {
				playtype_arm_ball_position = ball_->position();
				return playtype::execute_direct_free_kick_friendly;
			}

		case 'i': // INDIRECT FREE KICK YELLOW
			if (old_pt == playtype::play) {
				return playtype::play;
			} else if (old_pt == playtype::execute_indirect_free_kick_enemy) {
				if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return playtype::play;
				} else {
					return playtype::execute_indirect_free_kick_enemy;
				}
			} else {
				playtype_arm_ball_position = ball_->position();
				return playtype::execute_indirect_free_kick_enemy;
			}

		case 'I': // INDIRECT FREE KICK BLUE
			if (old_pt == playtype::play) {
				return playtype::play;
			} else if (old_pt == playtype::execute_indirect_free_kick_friendly) {
				if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return playtype::play;
				} else {
					return playtype::execute_indirect_free_kick_friendly;
				}
			} else {
				playtype_arm_ball_position = ball_->position();
				return playtype::execute_indirect_free_kick_friendly;
			}

		case 's': // FORCE START
			return playtype::play;

		case 'k': // KICKOFF YELLOW
			return playtype::prepare_kickoff_enemy;

		case 'K': // KICKOFF BLUE
			return playtype::prepare_kickoff_friendly;

		case 'p': // PENALTY YELLOW
			return playtype::prepare_penalty_enemy;

		case 'P': // PENALTY BLUE
			return playtype::prepare_penalty_friendly;

		case 'h': // HALF TIME
		case 't': // TIMEOUT YELLOW
		case 'T': // TIMEOUT BLUE
			return playtype::pit_stop;

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
		default:
			return old_pt;
	}
}

