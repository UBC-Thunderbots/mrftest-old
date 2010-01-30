#ifndef WORLD_TEAM_H
#define WORLD_TEAM_H

#include "world/player.h"
#include "world/playtype.h"

//
// A group of robots controlled by one AI.
//
class team : public byref {
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
		// Gets the current play type, from this team's point of view.
		//
		virtual playtype::playtype current_playtype() const = 0;

		//
		// A signal emitted when a robot is added to the team. The new robot was
		// added at the end of the team.
		//
		sigc::signal<void> &signal_robot_added() {
			return sig_robot_added;
		}

		//
		// A signal emitted when a robot is removed from the team. The first
		// parameter is the index number of the removed robot. The second is the
		// robot itself.
		//
		sigc::signal<void, unsigned int, robot::ptr> &signal_robot_removed() {
			return sig_robot_removed;
		}

		//
		// A signal emitted whenever the play type changes.
		//
		sigc::signal<void, playtype::playtype> &signal_playtype_changed() {
			return sig_playtype_changed;
		}

	private:
		sigc::signal<void> sig_robot_added;
		sigc::signal<void, unsigned int, robot::ptr> sig_robot_removed;
		sigc::signal<void, playtype::playtype> sig_playtype_changed;
};

//
// A group of robots controlled by one AI on this computer.
//
class controlled_team : public team {
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
		robot::ptr get_robot(std::size_t idx) {
			return get_player(idx);
		}

		//
		// A signal emitted when a player is added to the team. The new player was
		// added at the end of the team.
		//
		sigc::signal<void> &signal_player_added() {
			return sig_player_added;
		}

		//
		// A signal emitted when a player is removed from the team. The first
		// parameter is the index number of the removed player. The second is the
		// player itself.
		//
		sigc::signal<void, unsigned int, player::ptr> &signal_player_removed() {
			return sig_player_removed;
		}

	private:
		sigc::signal<void> sig_player_added;
		sigc::signal<void, unsigned int, player::ptr> sig_player_removed;
};

#endif

