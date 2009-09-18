#ifndef WORLD_TEAM_H
#define WORLD_TEAM_H

#include <cstddef>
#include <glibmm.h>
#include "util/byref.h"
#include "world/player.h"
#include "world/robot.h"

//
// A group of robots controlled by one AI.
//
class team : public virtual byref {
	public:
		//
		// A pointer to a team.
		//
		typedef Glib::RefPtr<team> ptr;

		//
		// The number of robots on the team.
		//
		virtual std::size_t size() const;

		//
		// Gets one robot on the team.
		//
		virtual Glib::RefPtr<robot> get_robot(std::size_t idx);

		//
		// Gets the team's score.
		//
		virtual unsigned int score() const;

		//
		// Gets the team opposing this one.
		//
		team::ptr other();
};

//
// A group of robots controlled by one AI on this computer.
//
class controlled_team : public virtual team {
	public:
		//
		// A pointer to a controllable_team.
		//
		typedef Glib::RefPtr<controlled_team> ptr;

		//
		// Gets one player on the team.
		//
		virtual Glib::RefPtr<player> get_player(std::size_t idx);

		//
		// Gets one robot on the team.
		//
		virtual Glib::RefPtr<robot> get_robot(std::size_t idx) {
			return get_player(idx);
		}
};

#endif

