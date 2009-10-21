#ifndef WORLD_TEAM_H
#define WORLD_TEAM_H

#include "util/byref.h"
#include "world/player.h"
#include "world/robot.h"
#include <cstddef>
#include <glibmm.h>
#include <sigc++/sigc++.h>

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
		virtual std::size_t size() const = 0;

		//
		// Gets one robot on the team.
		//
		virtual robot::ptr get_robot(std::size_t idx) = 0;

		//
		// Gets the team's score.
		//
		virtual unsigned int score() const = 0;

		//
		// Gets the team opposing this one.
		//
		virtual team::ptr other() = 0;

		//
		// Gets the colour of the team, true for yellow or false for blue.
		//
		virtual bool yellow() const = 0;

		//
		// A signal emitted when a robot is added to the team. The new robot was
		// added at the end of the team.
		//
		sigc::signal<void> &signal_robot_added() {
			return sig_robot_added;
		}

		//
		// A signal emitted when a robot is removed from the team. The parameter
		// is the index number of the removed robot.
		//
		sigc::signal<void, unsigned int> &signal_robot_removed() {
			return sig_robot_removed;
		}

	private:
		sigc::signal<void> sig_robot_added;
		sigc::signal<void, unsigned int> sig_robot_removed;
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
		virtual player::ptr get_player(std::size_t idx) = 0;

		//
		// Gets one robot on the team.
		//
		virtual robot::ptr get_robot(std::size_t idx) {
			return get_player(idx);
		}
};

#endif

