#include "ai/world/team.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>

namespace {
	class robot_comparator {
		public:
			bool operator()(robot::ptr bot1, robot::ptr bot2) const {
				if (bot1->yellow != bot2->yellow) {
					return bot2->yellow;
				} else {
					return bot1->pattern_index < bot2->pattern_index;
				}
			}
	};
}

team::team() {
}

enemy_team::ptr enemy_team::create() {
	ptr p(new enemy_team);
	return p;
}

void enemy_team::add(robot::ptr bot) {
	assert(bot);
	const std::vector<robot::ptr>::iterator i = std::lower_bound(members.begin(), members.end(), bot, robot_comparator());
	const unsigned int index = std::distance(members.begin(), i);
	members.insert(i, bot);
	signal_robot_added.emit(index, bot);
}

void enemy_team::remove(const unsigned int index) {
	assert(index < size());
	const robot::ptr bot(members[index]);
	members.erase(members.begin() + index);
	signal_robot_removed.emit(index, bot);
	if (bot->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of robot<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
	}
}

friendly_team::ptr friendly_team::create() {
	ptr p(new friendly_team);
	return p;
}

void friendly_team::add(player::ptr bot) {
	assert(bot);
	const std::vector<player::ptr>::iterator i = std::lower_bound(members.begin(), members.end(), bot, robot_comparator());
	const unsigned int index = std::distance(members.begin(), i);
	members.insert(i, bot);
	signal_robot_added.emit(index, bot);
	signal_player_added.emit(index, bot);
}

void friendly_team::remove(const unsigned int index) {
	assert(index < size());
	const player::ptr bot(members[index]);
	members.erase(members.begin() + index);
	signal_robot_removed.emit(index, bot);
	signal_player_removed.emit(index, bot);
	if (bot->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of player<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
	}
}

