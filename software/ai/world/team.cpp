#include "ai/world/team.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>

namespace {
	class RobotComparator {
		public:
			bool operator()(Robot::ptr bot1, Robot::ptr bot2) const {
				if (bot1->yellow != bot2->yellow) {
					return bot2->yellow;
				} else {
					return bot1->pattern_index < bot2->pattern_index;
				}
			}
	};
}

Team::Team() {
}

EnemyTeam::ptr EnemyTeam::create() {
	ptr p(new EnemyTeam);
	return p;
}

void EnemyTeam::add(Robot::ptr bot) {
	assert(bot);
	const std::vector<Robot::ptr>::iterator i = std::lower_bound(members.begin(), members.end(), bot, RobotComparator());
	const unsigned int index = std::distance(members.begin(), i);
	members.insert(i, bot);
	signal_robot_added.emit(index, bot);
}

void EnemyTeam::remove(const unsigned int index) {
	assert(index < size());
	const Robot::ptr bot(members[index]);
	members.erase(members.begin() + index);
	signal_robot_removed.emit(index, bot);
	if (bot->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of robot<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
	}
}

FriendlyTeam::ptr FriendlyTeam::create() {
	ptr p(new FriendlyTeam);
	return p;
}

void FriendlyTeam::add(Player::ptr bot) {
	assert(bot);
	const std::vector<Player::ptr>::iterator i = std::lower_bound(members.begin(), members.end(), bot, RobotComparator());
	const unsigned int index = std::distance(members.begin(), i);
	members.insert(i, bot);
	signal_robot_added.emit(index, bot);
	signal_player_added.emit(index, bot);
}

void FriendlyTeam::remove(const unsigned int index) {
	assert(index < size());
	const Player::ptr bot(members[index]);
	members.erase(members.begin() + index);
	signal_robot_removed.emit(index, bot);
	signal_player_removed.emit(index, bot);
	if (bot->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of player<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
	}
}

