#ifndef SIMULATOR_TEAM_H
#define SIMULATOR_TEAM_H

#include "ai/strategy.h"
#include "sim/engine.h"
#include "world/timestep.h"

class simulator;

//
// All the data about a physical team implemented by the simulator.
//
class simulator_team_data : public noncopyable {
	public:
		//
		// Constructs a new simulator_team_data.
		//
		simulator_team_data(simulator &sim, bool invert_playtype, xmlpp::Element *xml, bool yellow, ball::ptr ball, field::ptr field);

		//
		// Destroys a simulator_team_data.
		//
		~simulator_team_data();

		//
		// Sets the views of the other team.
		//
		void set_other(const team::ptr &w, const team::ptr &e) {
			assert(w);
			assert(e);
			west_other = w;
			east_other = e;
		}

		//
		// Sets the colour of this team.
		//
		void set_yellow(bool y) {
			yellow = y;
		}

		//
		// Gets the colour of this team.
		//
		bool is_yellow() {
			return yellow;
		}

		//
		// Gets the strategy governing this team.
		//
		strategy::ptr get_strategy() {
			return team_strategy;
		}

		//
		// Sets the strategy governing this team by name.
		//
		void set_strategy(const Glib::ustring &name);

		//
		// Configures the team to use a new engine.
		//
		void set_engine(simulator_engine::ptr e);

		//
		// Gets the controller factory currently in use.
		//
		robot_controller_factory *get_controller_type() const {
			return controller_factory;
		}

		//
		// Configures the team to use a class of controller.
		//
		void set_controller_type(const Glib::ustring &name);

		//
		// Adds a new player.
		//
		void add_player();

		//
		// Removes the player with the given index.
		//
		void remove_player(unsigned int index);

		//
		// Runs part of a time tick, before the engine is called. This should
		// only be called from the "simulator" class.
		//
		void tick_preengine() {
			if (team_strategy)
				team_strategy->tick();
			for (unsigned int i = 0; i < impls.size(); ++i)
				impls[i]->tick();
		}

		//
		// Runs part of a time tick, after the engine is called. This should
		// only be called from the "simulator" class.
		//
		void tick_postengine() {
			for (unsigned int i = 0; i < impls.size(); ++i)
				impls[i]->add_prediction_datum(impls[i]->position(), impls[i]->orientation(), 1.0 / TIMESTEPS_PER_SECOND);
		}

		//
		// Returns the current play type.
		//
		playtype::playtype current_playtype() const;

		//
		// Called by the simulator when the play type changes.
		//
		void playtype_changed(playtype::playtype);

		//
		// The simulator that holds this object.
		//
		simulator &the_simulator;

		//
		// Whether or not to invert the play types passed by the parent.
		//
		bool invert_playtype;

		//
		// Emitted when the play type changes.
		//
		sigc::signal<void, playtype::playtype> sig_playtype_changed;

		//
		// The current engine.
		//
		simulator_engine::ptr engine;

		//
		// The engine-provided implementations of the players.
		//
		std::vector<player_impl::ptr> impls;

		//
		// The west and east views of the players on this team.
		//
		std::vector<player::ptr> west_players, east_players;

		//
		// The score.
		//
		unsigned int score;

		//
		// The pointers to the west and east views of the other team.
		//
		team::ptr west_other, east_other;

		//
		// The colour of this team.
		//
		bool yellow;

		//
		// The current strategy governing this team.
		//
		strategy::ptr team_strategy;

		//
		// The robot controller class to use for new players.
		//
		robot_controller_factory *controller_factory;

	public:
		//
		// The objects that provide west and east views of this team.
		//
		const controlled_team::ptr west_view, east_view;

	private:
		//
		// The configuration element for this team.
		//
		xmlpp::Element *xml;

		//
		// The ball.
		//
		ball::ptr the_ball;

		//
		// The field.
		//
		field::ptr the_field;
};

//
// A view of a physical team from either the west or the east perspective.
//
class simulator_team_view : public controlled_team {
	public:
		//
		// Constructs a new simulator_team_view.
		//
		simulator_team_view(const simulator_team_data &data, const std::vector<player::ptr> &players, const team::ptr &other) : data(data), players(players), the_other(other) {
		}

		//
		// Gets the size of the team.
		//
		std::size_t size() const {
			return players.size();
		}

		//
		// Gets one player on the team.
		//
		player::ptr get_player(std::size_t idx) {
			assert(idx < players.size());
			return players[idx];
		}

		//
		// Gets the team's score.
		//
		unsigned int score() const {
			return data.score;
		}

		//
		// Gets the other team.
		//
		team::ptr other() {
			return the_other;
		}

		//
		// Gets the colour of the team.
		//
		bool yellow() const {
			return data.yellow;
		}

		//
		// Gets the current play type.
		//
		playtype::playtype current_playtype() const {
			return data.current_playtype();
		}

	private:
		//
		// The simulator_team_data backing this view.
		//
		const simulator_team_data &data;

		//
		// The vector, stored in the corresponding simulator_team_data object,
		// that contains the player objects exposed by this view.
		//
		const std::vector<player::ptr> &players;

		//
		// The pointer to the other team, stored in the corresponding
		// simulator_team_data object.
		//
		const team::ptr &the_other;
};

#endif

