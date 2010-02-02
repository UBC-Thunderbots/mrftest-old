#ifndef LOG_READER_TEAM_H
#define LOG_READER_TEAM_H

#include "log/reader/robot.h"
#include "world/robot.h"
#include "world/team.h"
#include <vector>
#include <glibmm.h>

//
// A team driven by a replay of a log file.
//
class log_reader_team : public team {
	public:
		//
		// A pointer to a log_reader_team.
		//
		typedef Glib::RefPtr<log_reader_team> ptr;

		//
		// Creates a new log_reader_team.
		//
		static ptr create(log_reader &reader) {
			ptr p(new log_reader_team(reader));
			return p;
		}

		//
		// Sets the other team.
		//
		void set_other(team::ptr other) {
			the_other = other;
		}

		//
		// Updates this team with data from a log file.
		//
		void update();

		team::ptr other() {
			return the_other;
		}

		std::size_t size() const {
			return lrrs.size();
		}

		robot::ptr get_robot(std::size_t i) {
			return bots[i];
		}

		unsigned int score() const {
			return the_score;
		}

		bool yellow() const {
			return is_yellow;
		}

		playtype::playtype current_playtype() const {
			return the_playtype;
		}

		log_reader &reader;
		std::vector<log_reader_robot::ptr> lrrs;
		std::vector<robot::ptr> bots;
		unsigned int the_score;
		bool is_yellow;
		team::ptr the_other;
		playtype::playtype the_playtype;

		log_reader_team(log_reader &r);
};

#endif

