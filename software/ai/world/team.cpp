#include "ai/world/team.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>

namespace {
	class RobotComparator {
		public:
			bool operator()(RefPtr<Robot> bot1, RefPtr<Robot> bot2) const {
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

RefPtr<EnemyTeam> EnemyTeam::create() {
	RefPtr<EnemyTeam> p(new EnemyTeam);
	return p;
}

void EnemyTeam::add(RefPtr<Robot> bot) {
	assert(bot);
	const std::vector<RefPtr<Robot> >::iterator i = std::lower_bound(members.begin(), members.end(), bot, RobotComparator());
	const unsigned int index = std::distance(members.begin(), i);
	members.insert(i, bot);
	signal_robot_added.emit(index, bot);
}

void EnemyTeam::remove(const unsigned int index) {
	assert(index < size());
	const RefPtr<Robot> bot(members[index]);
	members.erase(members.begin() + index);
	signal_robot_removed.emit(index, bot);
	if (bot->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of robot<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
	}
}

RefPtr<FriendlyTeam> FriendlyTeam::create() {
	RefPtr<FriendlyTeam> p(new FriendlyTeam);
	return p;
}

void FriendlyTeam::add(RefPtr<Player> bot) {
	assert(bot);
	const std::vector<RefPtr<Player> >::iterator i = std::lower_bound(members.begin(), members.end(), bot, RobotComparator());
	const unsigned int index = std::distance(members.begin(), i);
	members.insert(i, bot);
	signal_robot_added.emit(index, bot);
	signal_player_added.emit(index, bot);
}

void FriendlyTeam::remove(const unsigned int index) {
	assert(index < size());
	const RefPtr<Player> bot(members[index]);
	members.erase(members.begin() + index);
	signal_robot_removed.emit(index, bot);
	signal_player_removed.emit(index, bot);
	if (bot->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of player<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
	}
}

