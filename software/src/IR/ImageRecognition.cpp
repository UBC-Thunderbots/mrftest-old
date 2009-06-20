#include "AI/AITeam.h"
#include "AI/CentralAnalyzingUnit.h"
#include "datapool/Config.h"
#include "datapool/Field.h"
#include "datapool/Hungarian.h"
#include "datapool/Player.h"
#include "datapool/Team.h"
#include "datapool/World.h"
#include "IR/ImageRecognition.h"
#include "IR/messages_robocup_ssl_detection.pb.h"
#include "IR/messages_robocup_ssl_geometry.pb.h"
#include "IR/messages_robocup_ssl_wrapper.pb.h"
#include "Log/Log.h"

#include <algorithm>
#include <limits>
#include <cerrno>
#include <cstring>
#include <glibmm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define OFFSET_FROM_ROBOT_TO_BALL 80
#define INFINITE_DISTANCE 1.0e9

namespace {
	SSL_DetectionFrame detections[2];
}

ImageRecognition::ImageRecognition() : fd(-1), friendly(0), enemy(1) {
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

	friendly.side(false);
	enemy.side(true);
	World::init(friendly, enemy, field);

	World &w = World::get();

	//Set the player properties:
	for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
		w.player(i)->position(Vector2(INFINITE_DISTANCE, INFINITE_DISTANCE));
		w.player(i)->velocity(Vector2(0, 0));
		w.player(i)->acceleration(Vector2(0, 0));
		w.player(i)->radius(90);
	}

	//Set the ball properties:
	w.ball()->position(Vector2(330, 235));
	w.ball()->velocity(Vector2(0, 0));
	w.ball()->acceleration(Vector2(0, 0));
	w.ball()->radius(21.5);

	// Register for IO.
	Glib::signal_io().connect(sigc::mem_fun(*this, &ImageRecognition::onIO), fd, Glib::IO_IN);
}

bool ImageRecognition::onIO(Glib::IOCondition cond) {
	if (fd < 0)
		return false;

	if (cond & (Glib::IO_ERR | Glib::IO_NVAL | Glib::IO_HUP)) {
		Log::log(Log::LEVEL_ERROR, "IR") << "Error polling SSL-Vision socket.\n";
		return true;
	}

	if (!(cond & Glib::IO_IN))
		return true;

	char buffer[65536];
	ssize_t ret;
	ret = recv(fd, buffer, sizeof(buffer), 0);
	if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return true;
	} else if (ret < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "IR") << "Cannot receive data: " << std::strerror(err) << '\n';
		return true;
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
		detections[pkt.detection().camera_id()] = pkt.detection();

		// Handle the ball.
		{
			const SSL_DetectionBall *best = 0;
			for (unsigned int i = 0; i < sizeof(detections) / sizeof(*detections); i++) {
				for (int j = 0; j < detections[i].balls_size(); j++) {
					const SSL_DetectionBall &b = detections[i].balls(j);
					if (b.confidence() > 0.01 && (!best || b.confidence() > best->confidence()))
						best = &b;
				}
			}

			if (best) {
				World::get().ball()->position(Vector2(best->x(), -best->y()));
				World::get().isBallVisible(true);
			} else {
				World::get().isBallVisible(false);
			}
		}

		// Handle the robots.
		const struct ColourSet {
			char colour;
			const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &(SSL_DetectionFrame::*data)() const;
		} colourSets[2] = {
			{'B', &SSL_DetectionFrame::robots_blue},
			{'Y', &SSL_DetectionFrame::robots_yellow}
		};
		{
			const SSL_DetectionRobot *bestByID[2 * Team::SIZE] = {};
			std::vector<const SSL_DetectionRobot *> unidentified[2];
			for (unsigned int clr = 0; clr < 2; clr++) {
				for (unsigned int cam = 0; cam < 2; cam++) {
					const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &data = (detections[cam].*colourSets[clr].data)();
					for (int i = 0; i < data.size(); i++) {
						const SSL_DetectionRobot &bot = data.Get(i);
						if (bot.has_robot_id()) {
							unsigned int irid = bot.robot_id();
							std::ostringstream oss;
							oss << colourSets[clr].colour << irid;
							const std::string &key = oss.str();
							if (Config::instance().hasKey("IR2AI", key)) {
								unsigned int aiid = Config::instance().getInteger<unsigned int>("IR2AI", key, 10);
								if (!bestByID[aiid] || bestByID[aiid]->confidence() < bot.confidence())
									bestByID[aiid] = &bot;
							}
						} else {
							unidentified[clr].push_back(&bot);
						}
					}
				}
			}

			// Now:
			// bestByID has the highest-confidence pattern for each AI ID where patterns are provided, or null if none.
			// unidentified has all the detections that don't have known patterns.
			//
			// The bots whose positions we know can be updated immediately. The others go into a bipartite matching.
			std::vector<unsigned int> unusedAIIDs[2];
			for (unsigned int i = 0; i < 2 * Team::SIZE; i++)
				if (bestByID[i]) {
					PPlayer plr = World::get().player(i);
					plr->position(Vector2(bestByID[i]->x(), -bestByID[i]->y()));
					if (bestByID[i]->has_orientation()) {
						plr->orientation(bestByID[i]->orientation() / M_PI * 180.0);
					} else {
						plr->orientation(std::numeric_limits<double>::quiet_NaN());
					}
				} else {
					unusedAIIDs[i / Team::SIZE].push_back(i % Team::SIZE);
				}

			// Do bipartite matchings.
			for (unsigned int clr = 0; clr < 2; clr++) {
				if (!unidentified[clr].empty()) {
					std::ostringstream oss;
					oss << colourSets[clr].colour << 'H';
					const std::string &key = oss.str();
					unsigned int team = Config::instance().getInteger<unsigned int>("IR2AI", key, 10);
					Hungarian hung(std::max(unusedAIIDs[team].size(), unidentified[clr].size()));
					for (unsigned int x = 0; x < hung.size(); x++) {
						for (unsigned int y = 0; y < hung.size(); y++) {
							if (x < unidentified[clr].size() && y < unusedAIIDs[team].size()) {
								Vector2 oldPos = World::get().team(team).player(unusedAIIDs[team][y])->position();
								Vector2 newPos = Vector2(unidentified[clr][x]->x(), -unidentified[clr][x]->y());
								hung.weight(x, y) = INFINITE_DISTANCE - (newPos - oldPos).length();
							}
						}
					}
					hung.execute();
					for (unsigned int x = 0; x < unidentified[clr].size(); x++) {
						unsigned int y = hung.matchX(x);
						if (y < Team::SIZE) {
							const SSL_DetectionRobot &bot = *unidentified[clr][x];
							PPlayer plr = World::get().team(team).player(unusedAIIDs[team][y]);
							plr->position(Vector2(bot.x(), -bot.y()));
							if (bot.has_orientation()) {
								plr->orientation(bot.orientation() / M_PI * 180.0);
							} else {
								plr->orientation(std::numeric_limits<double>::quiet_NaN());
							}
						}
					}
				}
			}
		}

		const Vector2 &ballPos = World::get().ball()->position();
		std::vector<PPlayer> possessors;
		for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
			PPlayer pl = World::get().player(i);
			Vector2 playerPoint = pl->position();
			if (pl->hasDefiniteOrientation()) {
				playerPoint += OFFSET_FROM_ROBOT_TO_BALL * Vector2(pl->orientation());
			}
			if ((ballPos - playerPoint).length() < 17) {
				possessors.push_back(pl);
			}
		}
		if (!possessors.empty()) {
			PPlayer pl = possessors[0];
			pl->hasBall(true);
		} else {
			for (unsigned int i = 0; i < 2 * Team::SIZE; i++)
				World::get().player(i)->hasBall(false);
		}

		if (!World::get().isBallVisible()) {
			for (unsigned int i = 0; i < 2 * Team::SIZE; i++) {
				PPlayer pl = World::get().player(i);
				if (pl->hasBall()) {
					Vector2 ballPos = pl->position();
					if (pl->hasDefiniteOrientation()) {
						ballPos += OFFSET_FROM_ROBOT_TO_BALL * Vector2(pl->orientation());
					}
					World::get().ball()->position(ballPos);
					break;
				}
			}
		}
	}

	return true;
}

