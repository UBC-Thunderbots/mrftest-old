#include "IR/ImageRecognition.h"
#include "IR/messages_robocup_ssl_detection.pb.h"
#include "IR/messages_robocup_ssl_geometry.pb.h"
#include "IR/messages_robocup_ssl_wrapper.pb.h"
#include "AI/AITeam.h"
#include "Log/Log.h"
#include "datapool/Field.h"
#include "datapool/Player.h"
#include "datapool/Team.h"
#include "datapool/World.h"

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define OFFSET_FROM_ROBOT_TO_BALL 80

static bool seenBallFromCamera[2] = {false, false};

ImageRecognition::ImageRecognition() {
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	fd = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot create socket: " << std::strerror(err) << '\n';
		return;
	}

	int reuse = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot set socket option: " << std::strerror(err) << '\n';
		fd = -1;
		return;
	}

	union {
		sockaddr sa;
		sockaddr_in in;
	} sa;
	sa.in.sin_family = AF_INET;
	sa.in.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.in.sin_port = htons(10002);
	std::memset(&sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
	if (::bind(fd, &sa.sa, sizeof(sa.in)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot bind: " << std::strerror(err) << '\n';
		fd = -1;
		return;
	}

	ip_mreqn req;
	req.imr_multiaddr.s_addr = ::inet_addr("224.5.23.2");
	req.imr_address.s_addr = htonl(INADDR_ANY);
	req.imr_ifindex = 0;
	if (::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(req)) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot join multicast group: " << std::strerror(err) << '\n';
		fd = -1;
		return;
	}

	PGoal goalW = Goal::create(Vector2(25, 200),  Vector2(25, 270),  Vector2(75, 217.5),  Vector2(75, 252.5),  16, Vector2(70, 235));
	PGoal goalE = Goal::create(Vector2(635, 200), Vector2(635, 270), Vector2(585, 217.5), Vector2(585, 252.5), 16, Vector2(590, 235));
	PField field = Field::create(660, 470, 25, 635, 25, 445, Vector2(330, 235), 50, goalW, goalE);

	PTeam friendlyTeam = AITeam::create(0);
	friendlyTeam->side(true);

	PTeam enemyTeam = Team::create(1);
	enemyTeam->side(false);

	World::init(friendlyTeam, enemyTeam, field);

	World &w = World::get();

	//Set the player properties:
	w.player(0)->position(Vector2(400, 235));
	w.player(1)->position(Vector2(1070, 130));
	w.player(2)->position(Vector2(1070, 740));
	w.player(3)->position(Vector2(1070, -235));
	w.player(4)->position(Vector2(2500, 235));
	w.player(5)->position(Vector2(5150, -235));
	w.player(6)->position(Vector2(4030, 730));
	w.player(7)->position(Vector2(4030, -340));
	w.player(8)->position(Vector2(4030, 235));
	w.player(9)->position(Vector2(3000, -235));

	for (int i = 0; i < 10; i++) {
		w.player(i)->velocity(Vector2(0, 0));
		w.player(i)->acceleration(Vector2(0, 0));
		w.player(i)->radius(90);

	}

	//Set the ball properties:
	w.ball()->position(Vector2(330, 235));
	w.ball()->velocity(Vector2(0, 0));
	w.ball()->acceleration(Vector2(0, 0));
	w.ball()->radius(21.5);
}

ImageRecognition::~ImageRecognition() {
}

void ImageRecognition::update() {
	if (fd >= 0) {
		char buffer[65536];
		ssize_t ret;
		do {
			ret = ::recv(fd, buffer, sizeof(buffer), 0);
		} while (ret < 0 && errno == EINTR);
		if (ret < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "IR") << "Cannot receive data: " << std::strerror(err) << '\n';
			return;
		}

		SSL_WrapperPacket pkt;
		pkt.ParseFromArray(buffer, ret);
		if (pkt.has_geometry()) {
			const SSL_GeometryFieldSize &fData = pkt.geometry().field();

			PGoal goalW = World::get().field()->westGoal();
			goalW->north = Vector2(-fData.field_length() / 2.0, -fData.goal_width() / 2.0);
			goalW->south = Vector2(-fData.field_length() / 2.0, fData.goal_width() / 2.0);
			goalW->defenseN = Vector2(-fData.field_length() / 2.0 + fData.defense_radius(), -fData.defense_stretch() / 2.0);
			goalW->defenseS = Vector2(-fData.field_length() / 2.0 + fData.defense_radius(), fData.defense_stretch() / 2.0);
			goalW->height = 160;
			goalW->penalty = Vector2(-fData.field_length() / 2.0 + 450, 0);

			PGoal goalE = World::get().field()->eastGoal();
			goalE->north = Vector2(fData.field_length() / 2.0, -fData.goal_width() / 2.0);
			goalE->south = Vector2(fData.field_length() / 2.0, fData.goal_width() / 2.0);
			goalE->defenseN = Vector2(fData.field_length() / 2.0 - fData.defense_radius(), -fData.defense_stretch() / 2.0);
			goalE->defenseS = Vector2(fData.field_length() / 2.0 - fData.defense_radius(), fData.defense_stretch() / 2.0);
			goalE->height = 160;
			goalE->penalty = Vector2(fData.field_length() / 2.0 - 450, 0);

			PField f = World::get().field();
			f->width(fData.field_length());
			f->height(fData.field_width());
			f->west(-fData.field_length() / 2.0);
			f->east(fData.field_length() / 2.0);
			f->north(-fData.field_width() / 2.0);
			f->south(fData.field_width() / 2.0);
			f->centerCircle(Vector2(0, 0));
			f->centerCircleRadius(fData.center_circle_radius());
		}

		if (pkt.has_detection()) {
			const SSL_DetectionFrame &det = pkt.detection();
			if (det.balls_size()) {
				seenBallFromCamera[det.camera_id()] = true;
				const SSL_DetectionBall &ball = det.balls(0);
				if (ball.confidence() > 0.01)
					World::get().ball()->position(Vector2(ball.x(), -ball.y()));
			} else {
				seenBallFromCamera[det.camera_id()] = false;
			}

			for (int i = 0; i < det.robots_yellow_size(); i++) {
				const SSL_DetectionRobot &bot = det.robots_yellow(i);
				if (bot.has_robot_id()) {
					PPlayer player(World::get().friendlyTeam()->player(bot.robot_id()));
					player->position(Vector2(bot.x(), -bot.y()));
					if (bot.has_orientation()) {
						// SSL-Vision gives us radians; AI wants degrees.
						player->orientation(bot.orientation() / M_PI * 180.0);
					}
				}
			}

			for (int i = 0; i < det.robots_blue_size(); i++) {
				const SSL_DetectionRobot &bot = det.robots_blue(i);
				World::get().enemyTeam()->player(i)->position(Vector2(bot.x(), -bot.y()));
			}

			World::get().isBallVisible(seenBallFromCamera[0] || seenBallFromCamera[1]);
		}

		if (World::get().isBallVisible()) {
			std::vector<PPlayer> possessors;
			for (unsigned int i = 0; i < Team::SIZE; i++) {
				PPlayer pl = World::get().friendlyTeam()->players()[i];
				if ((World::get().ball()->position() - (pl->position() + OFFSET_FROM_ROBOT_TO_BALL * Vector2(pl->orientation()))).length() < 90) {
					possessors.push_back(pl);
				}
			}
			if (!possessors.empty()) {
				PPlayer pl = possessors[0];
				pl->hasBall(true);
			} else {
				for (unsigned int i = 0; i < Team::SIZE; i++)
					World::get().friendlyTeam()->player(i)->hasBall(false);
			}
		} else {
			for (unsigned int i = 0; i < Team::SIZE; i++) {
				PPlayer pl = World::get().friendlyTeam()->player(i);
				if (pl->hasBall()) {
					World::get().ball()->position(pl->position() + OFFSET_FROM_ROBOT_TO_BALL * Vector2(pl->orientation()));
					break;
				}
			}
		}
	}
}

