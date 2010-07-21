#include "proto/messages_robocup_ssl_detection.pb.h"
#include "proto/messages_robocup_ssl_geometry.pb.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "simulator/field.h"
#include "simulator/simulator.h"
#include "util/dprint.h"
#include "util/sockaddrs.h"
#include "util/xbee.h"
#include "xbee/shared/packettypes.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace {
	const uint16_t ADC_MAX = 1023;
	const double VCC = 3.3;
	const double DIVIDER_UPPER = 2200.0;
	const double DIVIDER_LOWER = 470.0;

	/**
	 * The number of milliseconds between receiving packet and sending its
	 * response.
	 */
	const unsigned int DELAY = 10;
}

Simulator::Simulator(const Config &conf, SimulatorEngine::Ptr engine, ClockSource &clk) : conf(conf), engine(engine), host_address16(0xFFFF), sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP), visdata(*this) {
	frame_counters[0] = frame_counters[1] = 0;
	int one = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) < 0) {
		throw std::runtime_error("Cannot enable broadcast on SSL-Vision socket.");
	}
	const Config::RobotSet &infos(conf.robots());
	for (unsigned int i = 0; i < infos.size(); ++i) {
		robots_[infos[i].address] = SimulatorRobot::create(infos[i], engine);
	}
	visdata.init();
	clk.signal_tick.connect(sigc::mem_fun(this, &Simulator::tick));
	Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &Simulator::tick_geometry), 1);
}

SimulatorRobot::Ptr Simulator::find_by16(uint16_t addr) const {
	for (std::unordered_map<uint64_t, SimulatorRobot::Ptr>::const_iterator i = robots_.begin(), iend = robots_.end(); i != iend; ++i) {
		if (i->second->address16() == addr) {
			return i->second;
		}
	}
	return SimulatorRobot::Ptr();
}

void Simulator::send(const iovec *iov, std::size_t iovlen) {
	// Coalesce buffers.
	std::size_t total_length = 0;
	for (unsigned int i = 0; i < iovlen; ++i) {
		total_length += iov[i].iov_len;
	}
	std::vector<uint8_t> data(total_length);
	total_length = 0;
	for (unsigned int i = 0; i < iovlen; ++i) {
		const uint8_t *src(static_cast<const uint8_t *>(iov[i].iov_base));
		std::copy(src, src + iov[i].iov_len, data.begin() + total_length);
		total_length += iov[i].iov_len;
	}

	// If we were to handle the packet immediately, then (1) it would be
	// unrealistic because a real radio doesn't do that, and (2) it would make
	// the simulator peg the CPU at 100% because you'd end up with the arbiter
	// dæmon spewing remote ATMY packets and the simulator hurling back NO_ACK
	// responses at full speed and making everything else slow and laggy. So
	// instead, just delay a few milliseconds before actually doing anything
	// with the packet.
	Glib::signal_timeout().connect_once(sigc::bind(sigc::mem_fun(this, &Simulator::packet_handler), data), 10);
}

void Simulator::packet_handler(const std::vector<uint8_t> &data) {
	// Dispatch packet based on API ID.
	if (data[0] == XBeePacketTypes::AT_REQUEST_APIID) {
		const XBeePacketTypes::AT_REQUEST<2> &req = *reinterpret_cast<const XBeePacketTypes::AT_REQUEST<2> *>(&data[0]);

		XBeePacketTypes::AT_RESPONSE resp;
		resp.apiid = XBeePacketTypes::AT_RESPONSE_APIID;
		resp.frame = req.frame;
		std::copy(&req.command[0], &req.command[2], &resp.command[0]);

		if (data.size() == sizeof(req) && req.command[0] == 'M' && req.command[1] == 'Y') {
			host_address16 = (req.value[0] << 8) | req.value[1];
			resp.status = XBeePacketTypes::AT_RESPONSE_STATUS_OK;
		} else {
			LOG_WARN(Glib::ustring::format("Received unsupported local AT command \"%1\".", Glib::ustring(reinterpret_cast<const char *>(req.command), 2)));
			resp.status = XBeePacketTypes::AT_RESPONSE_STATUS_INVALID_COMMAND;
		}

		if (req.frame) {
			queue_response(&resp, sizeof(resp));
		}
	} else if (data[0] == XBeePacketTypes::REMOTE_AT_REQUEST_APIID) {
		const XBeePacketTypes::REMOTE_AT_REQUEST<0> &req = *reinterpret_cast<const XBeePacketTypes::REMOTE_AT_REQUEST<0> *>(&data[0]);

		XBeePacketTypes::REMOTE_AT_RESPONSE resp;
		resp.apiid = XBeePacketTypes::REMOTE_AT_RESPONSE_APIID;
		resp.frame = req.frame;
		std::copy(&req.address64[0], &req.address64[8], &resp.address64[0]);
		std::copy(&req.address16[0], &req.address16[2], &resp.address16[0]);
		std::copy(&req.command[0], &req.command[2], &resp.command[0]);

		if (req.address16[0] == 0xFF && req.address16[1] == 0xFE) {
			uint64_t recipient = XBeeUtil::address_from_bytes(req.address64);
			std::unordered_map<uint64_t, SimulatorRobot::Ptr>::const_iterator i = robots_.find(recipient);
			if (i != robots_.end() && i->second->powered()) {
				if (data.size() == sizeof(req) + 2 && req.command[0] == 'M' && req.command[1] == 'Y') {
					uint16_t value = (req.value[0] << 8) | req.value[1];
					if (value) {
						i->second->address16(value);
						resp.status = XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK;
					} else {
						LOG_WARN("Please don't set a robot's 16-bit address to zero.");
						resp.status = XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER;
					}
				} else if (data.size() == sizeof(req) + 1 && req.command[0] == 'D' && req.command[1] == '0') {
					if (req.value[0] == 4) {
						i->second->bootloading(false);
						resp.status = XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK;
					} else if (req.value[0] == 5) {
						i->second->bootloading(true);
						resp.status = XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK;
					} else {
						LOG_WARN("Please don't set the bootload line to something other than high or low.");
						resp.status = XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER;
					}
				} else {
					LOG_WARN(Glib::ustring::compose("Received unsupported remote AT command \"%1\".", Glib::ustring(reinterpret_cast<const char *>(req.command), 2)));
					resp.status = XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND;
				}
			} else {
				resp.status = XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE;
			}
		} else {
			LOG_WARN("Please don't use 16-bit addresses to target remote AT commands.");
			resp.status = XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_ERROR;
		}

		if (req.frame) {
			queue_response(&resp, sizeof(resp));
		}
	} else if (data[0] == XBeePacketTypes::TRANSMIT16_APIID) {
		const struct __attribute__((packed)) TX16 {
			XBeePacketTypes::TRANSMIT16_HDR hdr;
			uint8_t data[];
		} &req = *reinterpret_cast<const TX16 *>(&data[0]);

		XBeePacketTypes::TRANSMIT_STATUS resp;
		resp.apiid = XBeePacketTypes::TRANSMIT_STATUS_APIID;
		resp.frame = req.hdr.frame;

		uint16_t recipient = (req.hdr.address[0] << 8) | req.hdr.address[1];
		if (data.size() == sizeof(req) + 1 && recipient != 0xFFFF) {
			SimulatorRobot::Ptr bot(find_by16(recipient));
			if (bot) {
				bot->run_data_offset(req.data[0]);
				resp.status = XBeePacketTypes::TRANSMIT_STATUS_SUCCESS;
			} else {
				if (req.hdr.options & XBeePacketTypes::TRANSMIT_OPTION_DISABLE_ACK) {
					resp.status = XBeePacketTypes::TRANSMIT_STATUS_SUCCESS;
				} else {
					resp.status = XBeePacketTypes::TRANSMIT_STATUS_NO_ACK;
				}
			}
		} else if (recipient == 0xFFFF) {
			for (std::unordered_map<uint64_t, SimulatorRobot::Ptr>::const_iterator i(robots_.begin()), iend(robots_.end()); i != iend; ++i) {
				SimulatorRobot::Ptr bot(i->second);
				if (bot->powered() && bot->address16() != 0xFFFF && sizeof(req.hdr) + bot->run_data_offset() + sizeof(XBeePacketTypes::RUN_DATA) <= data.size()) {
					const XBeePacketTypes::RUN_DATA &rundata = *reinterpret_cast<const XBeePacketTypes::RUN_DATA *>(&req.data[bot->run_data_offset()]);
					if (rundata.flags & XBeePacketTypes::RUN_FLAG_RUNNING) {
						SimulatorPlayer::Ptr plr(bot->get_player());
						if (plr) {
							plr->received(rundata);
						}
						if (rundata.flags & XBeePacketTypes::RUN_FLAG_FEEDBACK) {
							struct __attribute__((packed)) FEEDBACK {
								XBeePacketTypes::RECEIVE16_HDR hdr;
								XBeePacketTypes::FEEDBACK_DATA data;
							} fb;
							fb.hdr.apiid = XBeePacketTypes::RECEIVE16_APIID;
							fb.hdr.address[0] = bot->address16() >> 8;
							fb.hdr.address[1] = bot->address16() & 0xFF;
#warning implement inbound RSSI support
							fb.hdr.rssi = 40;
							fb.hdr.options = 0;
#warning implement chicker support
							fb.data.flags = XBeePacketTypes::FEEDBACK_FLAG_RUNNING;
#warning implement outbound RSSI support
							fb.data.outbound_rssi = 40;
							fb.data.dribbler_speed = plr ? plr->dribbler_speed() : 0;
							fb.data.battery_level = std::min(ADC_MAX, static_cast<uint16_t>(bot->battery() / (DIVIDER_LOWER + DIVIDER_UPPER) * DIVIDER_LOWER / VCC * ADC_MAX));
#warning implement fault support
							fb.data.faults = 0;
							queue_response(&fb, sizeof(fb));
						}
					}
				}
			}
			if (req.hdr.options & XBeePacketTypes::TRANSMIT_OPTION_DISABLE_ACK) {
				resp.status = XBeePacketTypes::TRANSMIT_STATUS_SUCCESS;
			} else {
				resp.status = XBeePacketTypes::TRANSMIT_STATUS_NO_ACK;
			}
		}

		if (req.hdr.frame) {
			queue_response(&resp, sizeof(resp));
		}
	} else {
		LOG_WARN(Glib::ustring::compose("Received unsupported XBee packet of type 0x%1.", tohex(data[0], 2)));
	}
}

void Simulator::queue_response(const void *buffer, std::size_t size) {
	const uint8_t *p(static_cast<const uint8_t *>(buffer));
	responses.push(std::vector<uint8_t>(p, p + size));
	if (!response_push_connection.connected()) {
		response_push_connection = Glib::signal_timeout().connect(sigc::mem_fun(this, &Simulator::push_response), DELAY);
	}
}

bool Simulator::push_response() {
	if (!responses.empty()) {
		signal_received.emit(responses.front());
		responses.pop();
	}
	return !responses.empty();
}

void Simulator::tick() {
	engine->tick();

	// Assume that each camera can see 10% of the other camera's area of the field.
	static const double LIMIT_MAG = SimulatorField::length / 2 * 0.1;
	static const double LIMIT_SIGNS[2] = { -1.0, +1.0 };

	for (unsigned int i = 0; i < 2; ++i) {
		SSL_WrapperPacket packet;
		SSL_DetectionFrame *det(packet.mutable_detection());
		det->set_frame_number(frame_counters[i]++);
#warning add times
		det->set_t_capture(0.0);
		det->set_t_sent(0.0);
		det->set_camera_id(i);
		SSL_DetectionBall *ball(det->add_balls());
		ball->set_confidence(1.0);
#warning add ball area
		const Point &ball_pos(engine->get_ball()->position());
		ball->set_x(ball_pos.x * 1000);
		ball->set_y(ball_pos.y * 1000);
#warning add pixel locations
		ball->set_pixel_x(0.0);
		ball->set_pixel_y(0.0);

		for (std::unordered_map<uint64_t, SimulatorRobot::Ptr>::const_iterator j(robots_.begin()), jend(robots_.end()); j != jend; ++j) {
			SimulatorRobot::Ptr bot(j->second);
			SimulatorPlayer::Ptr plr(bot->get_player());
			if (plr && plr->position().x * LIMIT_SIGNS[i] > -LIMIT_MAG) {
				SSL_DetectionRobot *elem;
				const Config::RobotInfo &info(conf.robots().find(j->first));
				if (info.yellow) {
					elem = det->add_robots_yellow();
				} else {
					elem = det->add_robots_blue();
				}
				elem->set_confidence(1.0);
				elem->set_robot_id(info.pattern_index);
				const Point &pos(plr->position());
				elem->set_x(pos.x * 1000);
				elem->set_y(pos.y * 1000);
				elem->set_orientation(plr->orientation());
#warning add pixel locations
				elem->set_pixel_x(0.0);
				elem->set_pixel_y(0.0);
#warning add height
			}
		}

		uint8_t buffer[packet.ByteSize()];
		packet.SerializeToArray(buffer, sizeof(buffer));

		SockAddrs sa;
		sa.in.sin_family = AF_INET;
		sa.in.sin_addr.s_addr = inet_addr("127.255.255.255");
		sa.in.sin_port = htons(10002);
		std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));

		if (sendto(sock, buffer, sizeof(buffer), MSG_NOSIGNAL, &sa.sa, sizeof(sa.in)) != static_cast<ssize_t>(sizeof(buffer))) {
			LOG_WARN("Error sending vision-type UDP packet.");
		}
	}

	visdata.signal_visdata_changed.emit();
}

bool Simulator::tick_geometry() {
	SSL_WrapperPacket packet;
	SSL_GeometryData *geom(packet.mutable_geometry());
	SSL_GeometryFieldSize *fld(geom->mutable_field());
	fld->set_line_width(10);
	fld->set_field_length(SimulatorField::length * 1000);
	fld->set_field_width(SimulatorField::width * 1000);
	fld->set_boundary_width(250);
	fld->set_referee_width((SimulatorField::total_length - SimulatorField::length) / 2 * 1000 - 250);
	fld->set_goal_width(SimulatorField::goal_width * 1000);
	fld->set_goal_depth(450);
	fld->set_goal_wall_width(10);
	fld->set_center_circle_radius(SimulatorField::centre_circle_radius * 1000);
	fld->set_defense_radius(SimulatorField::defense_area_radius * 1000);
	fld->set_defense_stretch(SimulatorField::defense_area_stretch * 1000);
	fld->set_free_kick_from_defense_dist(200);
	fld->set_penalty_spot_from_field_line_dist(450);
	fld->set_penalty_line_from_spot_dist(400);
	for (unsigned int i = 0; i < 2; ++i) {
		SSL_GeometryCameraCalibration *calib(geom->add_calib());
		calib->set_camera_id(0);
		calib->set_focal_length(500.0);
		calib->set_principal_point_x(390.0);
		calib->set_principal_point_y(290.0);
		calib->set_distortion(0.0);
		calib->set_q0(0.7);
		calib->set_q1(-0.7);
		calib->set_q2(0.0);
		calib->set_q3(0.0);
		calib->set_tx(0.0);
		calib->set_ty(1250.0);
		calib->set_tz(3500.0);
	}

	uint8_t buffer[packet.ByteSize()];
	packet.SerializeToArray(buffer, sizeof(buffer));

	SockAddrs sa;
	sa.in.sin_family = AF_INET;
	sa.in.sin_addr.s_addr = inet_addr("127.255.255.255");
	sa.in.sin_port = htons(10002);
	std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));

	if (sendto(sock, buffer, sizeof(buffer), MSG_NOSIGNAL, &sa.sa, sizeof(sa.in)) != static_cast<ssize_t>(sizeof(buffer))) {
		LOG_WARN("Error sending vision-type UDP packet.");
	}

	return true;
}

