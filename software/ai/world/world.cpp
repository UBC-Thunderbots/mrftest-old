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
	static const unsigned int MAX_VISION_FAILURES = 120;
}

World::World(const Config &conf, const std::vector<XBeeDriveBot::Ptr> &xbee_bots) : conf(conf), east_(false), refbox_yellow_(false), vision_socket(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), ball_(Ball::create()), xbee_bots(xbee_bots), playtype_(PlayType::HALT), playtype_override(PlayType::HALT), playtype_override_active(false), vis_view(this), ball_filter_(0) {
	vision_socket->set_blocking(false);
	const int one = 1;
	if (setsockopt(vision_socket->fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		throw std::runtime_error("Cannot set SO_REUSEADDR.");
	}
	SockAddrs sa;
	sa.in.sin_family = AF_INET;
	sa.in.sin_addr.s_addr = get_inaddr_any();
	sa.in.sin_port = htons(10002);
	std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
	if (bind(vision_socket->fd(), &sa.sa, sizeof(sa.in)) < 0) {
		throw std::runtime_error("Cannot bind to port 10002 for vision data.");
	}
	ip_mreqn mcreq;
	mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.2");
	mcreq.imr_address.s_addr = get_inaddr_any();
	mcreq.imr_ifindex = 0;
	if (setsockopt(vision_socket->fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
		LOG_INFO("Cannot join multicast group 224.5.23.2 for vision data.");
	}
	Glib::signal_io().connect(sigc::mem_fun(this, &World::on_vision_readable), vision_socket->fd(), Glib::IO_IN);

	refbox_.signal_command_changed.connect(sigc::mem_fun(this, &World::update_playtype));

	timespec_now(playtype_time_);
}

void World::flip_ends() {
	// Change the flag.
	east_ = !east_;

	// Swap the ball.
	ball_->sign = east_ ? -1 : 1;
	ball_->clear_prediction(-ball_->position(), angle_mod(-ball_->orientation()));

	// Swap the robots.
	static Team * const teams[2] = { &friendly, &enemy };
	for (unsigned int i = 0; i < 2; ++i) {
		const Team &tm(*teams[i]);
		for (unsigned int j = 0; j < tm.size(); ++j) {
			Robot::Ptr bot(tm.get_robot(j));
			bot->sign = east_ ? -1 : 1;
			bot->clear_prediction(-bot->position(), angle_mod(bot->orientation() + M_PI));
		}
	}

	// Fire the signal.
	signal_flipped_ends.emit();
}

void World::flip_refbox_colour() {
	// Update the flag.
	refbox_yellow_ = !refbox_yellow_;

	// Notify listeners.
	signal_flipped_refbox_colour.emit();

	// Flip the current play type, so that the updater will flip it back and
	// have the proper "old" value.
	playtype_ = PlayType::INVERT[playtype_];

	// Now run the updater.
	update_playtype();
}

void World::ball_filter(BallFilter *filt) {
	if (ball_filter_ != filt) {
		LOG_DEBUG(Glib::ustring::compose("Changing to ball filter %1.", filt ? filt->name : "<None>"));
		ball_filter_ = filt;
	}
}

void World::tick_timestamp() {
	++timestamp_;
}

bool World::on_vision_readable(Glib::IOCondition) {
	// Receive a packet.
	uint8_t buffer[65536];
	ssize_t len = recv(vision_socket->fd(), buffer, sizeof(buffer), 0);
	if (len < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			LOG_WARN("Cannot receive packet from SSL-Vision.");
		}
		return true;
	}

	// Decode it.
	SSL_WrapperPacket packet;
	if (!packet.ParseFromArray(buffer, len)) {
		LOG_WARN("Received malformed SSL-Vision packet.");
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
			LOG_WARN(Glib::ustring::compose("Received SSL-Vision packet for unknown camera %1.", det.camera_id()));
			return true;
		}

		// Keep a local copy of all detection frames.
		detections[det.camera_id()].CopyFrom(det);

		// Update the ball.
		{
			// Build a vector of all detections so far.
			std::vector<std::pair<double, Point> > balls;
			for (unsigned int i = 0; i < 2; ++i) {
				for (int j = 0; j < detections[i].balls_size(); ++j) {
					const SSL_DetectionBall &b(detections[i].balls(j));
					balls.push_back(std::make_pair(b.confidence(), Point(b.x() / 1000.0, b.y() / 1000.0)));
				}
			}

			// Execute the current ball filter.
			Point pos;
			if (ball_filter_) {
				pos = ball_filter_->filter(balls, friendly, enemy);
			}

			// Use the result.
			ball_->update(pos);
		}

		// Update the robots.
		{
			std::vector<bool> used_data[2];
			static Team * const teams[2] = { &friendly, &enemy };
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
							Team &tm(*teams[m]);
							for (unsigned int n = 0; n < tm.size(); ++n) {
								Robot::Ptr bot(tm.get_robot(n));
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
				Team &tm(*teams[i]);
				for (unsigned int j = 0; j < tm.size(); ++j) {
					Robot::Ptr bot(tm.get_robot(j));
					if (!bot->seen_this_frame) {
						++bot->vision_failures;
					} else {
						bot->vision_failures = 0;
					}
					bot->seen_this_frame = false;
					if (bot->vision_failures >= MAX_VISION_FAILURES) {
						Player::Ptr plr(Player::Ptr::cast_dynamic(bot));
						if (plr.is()) {
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
							XBeeDriveBot::Ptr xbeebot;
							Glib::ustring name;
							for (unsigned int m = 0; m < conf.robots().size(); ++m) {
								if (conf.robots()[m].yellow == colour && conf.robots()[m].pattern_index == pattern_index) {
									xbeebot = xbee_bots[m];
									name = conf.robots()[m].name;
								}
							}
							if (xbeebot.is()) {
								Player::Ptr plr(Player::create(name, colour, pattern_index, xbeebot));
								plr->sign = east_ ? -1 : 1;
								plr->update(detbot);
								friendly.add(plr);
							} else {
								Robot::Ptr bot(Robot::create(colour, pattern_index));
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
	// Predictable and hence implement those functions as estimates based on a
	// delta time, we need to lock the prediction timestamps of those objects
	// before handing them to the visualizer for rendering, otherwise we won't
	// actually see them move!
	{
		static Team * const teams[2] = { &friendly, &enemy };
		for (unsigned int i = 0; i < 2; ++i) {
			const Team &tm(*teams[i]);
			for (unsigned int j = 0; j < tm.size(); ++j) {
				const Robot::Ptr bot(tm.get_robot(j));
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

void World::override_playtype(PlayType::PlayType pt) {
	if (pt != playtype_override || !playtype_override_active) {
		playtype_override = pt;
		playtype_override_active = true;
		update_playtype();
	}
}

void World::clear_playtype_override() {
	if (playtype_override_active) {
		playtype_override_active = false;
		update_playtype();
	}
}

double World::playtype_time() const {
	timespec now;
	timespec_now(now);
	timespec diff;
	timespec_sub(now, playtype_time_, diff);
	return timespec_to_double(diff);
}

void World::update_playtype() {
	PlayType::PlayType old_pt = playtype_;
	if (refbox_yellow_) {
		old_pt = PlayType::INVERT[old_pt];
	}
	PlayType::PlayType new_pt = compute_playtype(old_pt);
	if (refbox_yellow_) {
		new_pt = PlayType::INVERT[new_pt];
	}
	if (new_pt != playtype_) {
		LOG_DEBUG(Glib::ustring::compose("Play type changed to %1.", PlayType::DESCRIPTIONS_GENERIC[new_pt]));
		playtype_ = new_pt;
		signal_playtype_changed.emit();

		timespec_now(playtype_time_);
	}
}

PlayType::PlayType World::compute_playtype(PlayType::PlayType old_pt) {
	if (playtype_override_active) {
		return playtype_override;
	}

	switch (refbox_.command()) {
		case 'H': // HALT
		case 'h': // HALF TIME
		case 't': // TIMEOUT YELLOW
		case 'T': // TIMEOUT BLUE
			return PlayType::HALT;

		case 'S': // STOP
		case 'z': // END TIMEOUT
			return PlayType::STOP;

		case ' ': // NORMAL START
			switch (old_pt) {
				case PlayType::PREPARE_KICKOFF_FRIENDLY:
					playtype_arm_ball_position = ball_->position();
					return PlayType::EXECUTE_KICKOFF_FRIENDLY;

				case PlayType::PREPARE_KICKOFF_ENEMY:
					playtype_arm_ball_position = ball_->position();
					return PlayType::EXECUTE_KICKOFF_ENEMY;

				case PlayType::PREPARE_PENALTY_FRIENDLY:
					playtype_arm_ball_position = ball_->position();
					return PlayType::EXECUTE_PENALTY_FRIENDLY;

				case PlayType::PREPARE_PENALTY_ENEMY:
					playtype_arm_ball_position = ball_->position();
					return PlayType::EXECUTE_PENALTY_ENEMY;

				case PlayType::EXECUTE_KICKOFF_FRIENDLY:
				case PlayType::EXECUTE_KICKOFF_ENEMY:
				case PlayType::EXECUTE_PENALTY_FRIENDLY:
				case PlayType::EXECUTE_PENALTY_ENEMY:
					if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
						return PlayType::PLAY;
					} else {
						return old_pt;
					}

				default:
					return PlayType::PLAY;
			}

		case 'f': // DIRECT FREE KICK YELLOW
			if (old_pt == PlayType::PLAY) {
				return PlayType::PLAY;
			} else if (old_pt == PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) {
				if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return PlayType::PLAY;
				} else {
					return PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;
				}
			} else {
				playtype_arm_ball_position = ball_->position();
				return PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;
			}

		case 'F': // DIRECT FREE KICK BLUE
			if (old_pt == PlayType::PLAY) {
				return PlayType::PLAY;
			} else if (old_pt == PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) {
				if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return PlayType::PLAY;
				} else {
					return PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;
				}
			} else {
				playtype_arm_ball_position = ball_->position();
				return PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;
			}

		case 'i': // INDIRECT FREE KICK YELLOW
			if (old_pt == PlayType::PLAY) {
				return PlayType::PLAY;
			} else if (old_pt == PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY) {
				if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return PlayType::PLAY;
				} else {
					return PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;
				}
			} else {
				playtype_arm_ball_position = ball_->position();
				return PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;
			}

		case 'I': // INDIRECT FREE KICK BLUE
			if (old_pt == PlayType::PLAY) {
				return PlayType::PLAY;
			} else if (old_pt == PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY) {
				if ((ball_->position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
					return PlayType::PLAY;
				} else {
					return PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
				}
			} else {
				playtype_arm_ball_position = ball_->position();
				return PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
			}

		case 's': // FORCE START
			return PlayType::PLAY;

		case 'k': // KICKOFF YELLOW
			return PlayType::PREPARE_KICKOFF_ENEMY;

		case 'K': // KICKOFF BLUE
			return PlayType::PREPARE_KICKOFF_FRIENDLY;

		case 'p': // PENALTY YELLOW
			return PlayType::PREPARE_PENALTY_ENEMY;

		case 'P': // PENALTY BLUE
			return PlayType::PREPARE_PENALTY_FRIENDLY;

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

