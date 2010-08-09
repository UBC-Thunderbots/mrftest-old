#include "ai/world/team.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>

namespace {
	class RobotComparator {
		public:
			bool operator()(Robot::Ptr bot1, Robot::Ptr bot2) const {
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

EnemyTeam::Ptr EnemyTeam::create() {
	Ptr p(new EnemyTeam);
	return p;
}

void EnemyTeam::add(Robot::Ptr bot) {
	assert(bot.is());
	const std::vector<Robot::Ptr>::iterator i = std::lower_bound(members.begin(), members.end(), bot, RobotComparator());
	const unsigned int index = std::distance(members.begin(), i);
	members.insert(i, bot);
	signal_robot_added.emit(index, bot);
}

void EnemyTeam::remove(const unsigned int index) {
	assert(index < size());
	const Robot::Ptr bot(members[index]);
	members.erase(members.begin() + index);
	signal_robot_removed.emit(index, bot);
	if (bot->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of robot<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
	}
}

FriendlyTeam::Ptr FriendlyTeam::create() {
	Ptr p(new FriendlyTeam);
	return p;
}

void FriendlyTeam::add(Player::Ptr bot) {
	assert(bot.is());
	const std::vector<Player::Ptr>::iterator i = std::lower_bound(members.begin(), members.end(), bot, RobotComparator());
	const unsigned int index = std::distance(members.begin(), i);
	members.insert(i, bot);
	signal_robot_added.emit(index, bot);
	signal_player_added.emit(index, bot);
}

void FriendlyTeam::remove(const unsigned int index) {
	assert(index < size());
	const Player::Ptr bot(members[index]);
	members.erase(members.begin() + index);
	signal_robot_removed.emit(index, bot);
	signal_player_removed.emit(index, bot);
	if (bot->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of player<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
	}
}

