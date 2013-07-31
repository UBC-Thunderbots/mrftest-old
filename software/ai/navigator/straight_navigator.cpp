#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "geom/util.h"
#include "geom/point.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;

/*
 * version 0.01: account for path intersections
 * version 0.02: account for path obsticale intersections
 * version 0.10: account for flags
 * TODO: built-in hysterisis
*/

namespace {
	/**
	 * Staight Navigator
	 * The main functions that are required to be able to implement a navigator.
	 */
	class StraightNavigator : public Navigator {
		public:
			StraightNavigator(World world);
			void tick();
			NavigatorFactory &factory() const;

		private:
			void teamUpdater();
			void dealWithFlags();
			void dealWithPath();
			std::vector<Player> getMovePrioPlayers(AI::Flags::MovePrio);
			std::vector<Player> getMoveTypePlayers(AI::Flags::MoveType);
			std::vector<Player> getUnhappyPlayers();

			std::vector<bool> player_happiness;
			std::vector<AI::Flags::MoveType> movetype_array;
			std::vector<AI::Flags::MovePrio> moveprio_array;
	};

	typedef std::pair<Point, Point> SPath;
	typedef std::pair<Point, double> Circle;

	class PathModerator {
		public:
			PathModerator();
			void setPath(const std::vector<SPath> &all_path);
			void setObs(const std::vector<Circle> &all_obs);
			std::vector<std::pair<std::size_t, std::size_t>> getPathCrossPath(); // return index

		private:
			std::vector<SPath> _all_path;
			std::vector<Circle> _all_obs;
	};
}

PathModerator::PathModerator() {
}

void PathModerator::setPath(const std::vector<SPath> &all_path) {
	_all_path = all_path;
}

void PathModerator::setObs(const std::vector<Circle> &all_obs) {
	_all_obs = all_obs;
}

// get all path that crosses one another
std::vector<std::pair<std::size_t, std::size_t>> PathModerator::getPathCrossPath() {
	std::vector<std::pair<std::size_t, std::size_t>> crossing_path_index;
	for (std::size_t i = 0; i < _all_path.size(); i++) {
		for (std::size_t j = 0; j < i; j++) {
			SPath a = _all_path[i];
			SPath b = _all_path[j];
			if (seg_crosses_seg(a.first, a.second, b.first, b.second)) {
				std::pair<std::size_t, std::size_t> new_item(i, j);
				crossing_path_index.push_back(new_item);
			}
		}
	}
	return crossing_path_index;
}


StraightNavigator::StraightNavigator(World world) : Navigator(world) {
}

std::vector<Player> StraightNavigator::getMovePrioPlayers(AI::Flags::MovePrio prio) {
	std::vector<Player> interested_players;
	for (const Player i : world.friendly_team()) {
		if (i.prio() == prio) {
			interested_players.push_back(i);
		}
	}
	return interested_players;
}

std::vector<Player> StraightNavigator::getMoveTypePlayers(AI::Flags::MoveType type) {
	std::vector<Player> interested_players;
	for (const Player i : world.friendly_team()) {
		if (i.type() == type) {
			interested_players.push_back(i);
		}
	}
	return interested_players;
}

std::vector<Player> StraightNavigator::getUnhappyPlayers() {
	FriendlyTeam fteam = world.friendly_team();
	std::vector<Player> interested_players;
	for (std::size_t i = 0; i < fteam.size(); i++) {
		if (!player_happiness[i]) {
			interested_players.push_back(fteam[i]);
		}
	}
	return interested_players;
}

void StraightNavigator::teamUpdater() {
	player_happiness.clear();
	player_happiness.resize(world.friendly_team().size(), false);

	movetype_array.clear();
	moveprio_array.clear();
	for (Player i : world.friendly_team()) {
		movetype_array.push_back(i.type());
		moveprio_array.push_back(i.prio());
	}
}

void StraightNavigator::dealWithFlags() {
	// not dealing with flags yet
}

void StraightNavigator::dealWithPath() {
	std::vector<Player> unhappyPlayers = getUnhappyPlayers();
	std::vector<SPath> all_path;
	std::vector<Circle> all_obs;
	// put all path into the same variable
	for (std::size_t i = 0; i < unhappyPlayers.size(); i++) {
		SPath new_item(unhappyPlayers[i].position(), unhappyPlayers[i].destination().first);
		all_path.push_back(new_item);
	}
	PathModerator moderator;
	moderator.setPath(all_path);
	moderator.setObs(all_obs);

	// get all crossed path and deal with them
	const std::vector<std::pair<std::size_t, std::size_t>> &bot_crosses_bot = moderator.getPathCrossPath();
	for (auto i = bot_crosses_bot.begin(), iend = bot_crosses_bot.end(); i != iend; ++i) {
		Player botA = unhappyPlayers[i->first];
		Player botB = unhappyPlayers[i->second];
		Point intersect = line_intersect(botA.position(), botA.destination().first, botB.position(), botB.destination().first);
		if ((intersect - botA.position()).len() - (intersect-botB.position()).len() < Robot::MAX_RADIUS * 2) {
			if (botA.prio() == botB.prio()) {
				// let A get the priority
			} else if (botA.prio() == MovePrio::HIGH) {
				// let A get the priority
			} else if (botB.prio()	== MovePrio::HIGH) {
			} else if (botA.prio() == MovePrio::MEDIUM) {
			} else if (botB.prio() == MovePrio::MEDIUM) {
			}
		}
	}
}

void StraightNavigator::tick() {
	for (Player player : world.friendly_team()) {
		Point currentPosition = player.position();
		Angle currentOrientation = player.orientation();
		Point destinationPosition = player.destination().first;
		Angle destinationOrientation = player.destination().second;

		AI::Timestamp ts = get_next_ts(world.monotonic_time(), currentPosition, destinationPosition, player.target_velocity());

		Player::Path path;
		path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), ts));
		player.path(path);
	}
}

NAVIGATOR_REGISTER(StraightNavigator)

