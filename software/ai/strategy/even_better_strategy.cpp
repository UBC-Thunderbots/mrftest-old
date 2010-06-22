#include "ai/strategy/basic_strategy.h"
#include "ai/tactic/chase.h"
#include "ai/role/defensive2.h"
#include "ai/role/goalie.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_free_kick.h" 
#include "ai/role/pit_stop.h"   
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/kickoff_friendly.h"             
#include "ai/role/penalty_friendly.h"
#include "ai/role/penalty_enemy.h"
#include "ai/role/victory_dance.h"
#include "ai/util.h"

#include <iostream>
#include <vector>

namespace {

	/**
	 * A really basic strategy that satisfies the rule.
	 * Most strategies should derive this function and overide in_play_assignment().
	 * Assumes that the first robot is always the goalie.
	 */
	class even_better_strategy : public basic_strategy {
		public:
			even_better_strategy(world::ptr w);

			strategy_factory &get_factory();
			Gtk::Widget *get_ui_controls();

		protected:

			void in_play_assignment();

			player::ptr minus_one_assignment();
	};


	class even_better_strategy_factory : public strategy_factory {
		public:
			even_better_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	even_better_strategy_factory::even_better_strategy_factory() : strategy_factory("Even Better than Better Strategy") {
	}

	strategy::ptr even_better_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new even_better_strategy(world));
		return s;
	}

	even_better_strategy_factory factory;

	even_better_strategy::even_better_strategy(world::ptr w) : basic_strategy(w) {
	}

	Gtk::Widget *even_better_strategy::get_ui_controls() {
		return NULL;
	}

	void even_better_strategy::in_play_assignment() {
		const friendly_team &the_team(the_world->friendly);
		const point ballpos = the_world->ball()->position();

		roles.clear();
		if (the_team.size() == 0) return;

		// const vector<player::ptr> players = the_team.get_players();

		defensive2::ptr defensive_role(new defensive2(the_world));
		offensive::ptr offensive_role(new offensive(the_world));
		roles.push_back(role::ptr(defensive_role));
		roles.push_back(role::ptr(offensive_role));
		std::vector<player::ptr> defenders;
		std::vector<player::ptr> offenders;

		// TODO: change the following
		// For now, goalie is always lowest numbered robot
		int goalie_index = 0;
		defenders.push_back(the_team.get_player(goalie_index));

		int baller = -1;
		int baller_ignore_chicker = -1;
		if (the_team.size() >= 2){
			double best_ball_dist = 1e50;
			double best_ball_dist_ignore_chicker = 1e50;
			for (size_t i = 0; i < the_team.size(); i++){
				if (static_cast<int>(i) == goalie_index)
					continue;
				double ball_dist = (ballpos - the_team.get_player(i)->position()).len();
				if (ball_dist < best_ball_dist_ignore_chicker){
					baller_ignore_chicker = i;
					best_ball_dist_ignore_chicker = ball_dist;
				}
				if (the_team.get_player(i)->chicker_ready_time() >= player::CHICKER_FOREVER)
					continue;
				if (ball_dist < best_ball_dist){
					baller = i;
					best_ball_dist = ball_dist;
				}
			}
			if (baller != -1)
				offenders.push_back(the_team.get_player(baller));
			else{ // unlikely case that all robots have chicker faults
				offenders.push_back(the_team.get_player(baller_ignore_chicker));
			}
		}

		std::vector<player::ptr> rem_players;
		for (size_t i = 0; i < the_team.size(); i++){
			if (static_cast<int>(i) == goalie_index) continue;
			if (static_cast<int>(i) == baller) continue;
			if (baller == -1 && static_cast<int>(i) == baller_ignore_chicker) continue;
			rem_players.push_back(the_team.get_player(i));
		}
		std::sort(rem_players.begin(), rem_players.end(), ai_util::cmp_dist<player::ptr>(point(-the_world->field().length()/2.0,0.0)));

		// preferred_offender_number includes the offender assigned above (closest player to ball)
		// 3 players => 1 offender
		// 4 players => 1 offender
		// 5 players => 2 offenders;
		int preferred_offender_number = std::max(1, static_cast<int>(the_team.size()) - 3);
		switch(the_world->playtype()){
			case playtype::execute_direct_free_kick_enemy:
			case playtype::execute_indirect_free_kick_enemy:
			case playtype::prepare_kickoff_enemy:
			case playtype::execute_kickoff_enemy:
			case playtype::prepare_penalty_enemy:
			case playtype::execute_penalty_enemy:
				preferred_offender_number--;
				if (preferred_offender_number < 1)
					preferred_offender_number = 1;
				break;
			default:
				if (the_team.size() >= 5 && ai_util::friendly_posses_ball(the_world))
					preferred_offender_number++;
		}
		// preferred_defender_number includes goalie
		int preferred_defender_number = the_team.size() - preferred_offender_number;
		//TODO: assign robots not only based on distance to friendly goal, but also on chicker faults
		size_t i;
		for (i = 0; defenders.size() < preferred_defender_number; i++){
			defenders.push_back(rem_players[i]);
		}
		for (; i < rem_players.size(); i++){
			offenders.push_back(rem_players[i]);
		}
		defensive_role->set_robots(defenders);
		defensive_role->set_goalie(the_team.get_player(goalie_index));
		offensive_role->set_robots(offenders);
	}

	player::ptr even_better_strategy::minus_one_assignment() {
		const friendly_team &the_team(the_world->friendly);
		const point ballpos = the_world->ball()->position();

		roles.clear();
		if (the_team.size() == 0) return player::ptr();

		if (the_team.size() == 1) return the_team.get_player(0);

		// const vector<player::ptr> players = the_team.get_players();

		defensive2::ptr defensive_role(new defensive2(the_world));
		offensive::ptr offensive_role(new offensive(the_world));
		roles.push_back(role::ptr(defensive_role));
		roles.push_back(role::ptr(offensive_role));
		std::vector<player::ptr> defenders;
		std::vector<player::ptr> offenders;

		// TODO: change the following
		// For now, goalie is always lowest numbered robot
		int goalie_index = 0;
		defenders.push_back(the_team.get_player(goalie_index));

		int baller = -1;
		int baller_ignore_chicker = -1;
		if (the_team.size() >= 2){
			double best_ball_dist = 1e50;
			double best_ball_dist_ignore_chicker = 1e50;
			for (size_t i = 0; i < the_team.size(); i++){
				if (static_cast<int>(i) == goalie_index)
					continue;
				double ball_dist = (ballpos - the_team.get_player(i)->position()).len();
				if (ball_dist < best_ball_dist_ignore_chicker){
					baller_ignore_chicker = i;
					best_ball_dist_ignore_chicker = ball_dist;
				}
				if (the_team.get_player(i)->chicker_ready_time() >= player::CHICKER_FOREVER)
					continue;
				if (ball_dist < best_ball_dist){
					baller = i;
					best_ball_dist = ball_dist;
				}
			}
		}

		std::vector<player::ptr> rem_players;
		for (size_t i = 0; i < the_team.size(); i++){
			if (static_cast<int>(i) == goalie_index) continue;
			if (static_cast<int>(i) == baller) continue;
			if (baller == -1 && static_cast<int>(i) == baller_ignore_chicker) continue;
			rem_players.push_back(the_team.get_player(i));
		}
		std::sort(rem_players.begin(), rem_players.end(), ai_util::cmp_dist<player::ptr>(point(-the_world->field().length()/2.0,0.0)));

		// preferred_offender_number includes the assigned kicker (closest player to ball)
		// 3 players => 1 offender
		// 4 players => 1 offender
		// 5 players => 2 offenders;
		int preferred_offender_number = std::max(1, static_cast<int>(the_team.size()) - 3);
		switch(the_world->playtype()){
			case playtype::execute_direct_free_kick_enemy:
			case playtype::execute_indirect_free_kick_enemy:
			case playtype::prepare_kickoff_enemy:
			case playtype::execute_kickoff_enemy:
			case playtype::prepare_penalty_enemy:
			case playtype::execute_penalty_enemy:
				preferred_offender_number--;
				if (preferred_offender_number < 1)
					preferred_offender_number = 1;
				break;
			default:
				if (the_team.size() >= 5 && ai_util::friendly_posses_ball(the_world))
					preferred_offender_number++;
		}
		// preferred_defender_number includes goalie
		int preferred_defender_number = the_team.size() - preferred_offender_number;
		//TODO: assign robots not only based on distance to friendly goal, but also on chicker faults
		size_t i;
		for (i = 0; defenders.size() < preferred_defender_number; i++){
			defenders.push_back(rem_players[i]);
		}
		for (; i < rem_players.size(); i++){
			offenders.push_back(rem_players[i]);
		}
		defensive_role->set_robots(defenders);
		defensive_role->set_goalie(the_team.get_player(goalie_index));
		offensive_role->set_robots(offenders);

		if (baller != -1)
			return the_team.get_player(baller);
		else  // unlikely case that all robots have chicker faults
			return the_team.get_player(baller_ignore_chicker);
	}

	strategy_factory &even_better_strategy::get_factory() {
		return factory;
	}

}

