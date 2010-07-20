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
	 * A really basic Strategy that satisfies the rule.
	 * Most strategies should derive this function and overide in_play_assignment().
	 * Assumes that the first robot is always the goalie.
	 */
	class EvenBetterStrategy : public BasicStrategy {
		public:
			EvenBetterStrategy(RefPtr<World> w);

			StrategyFactory &get_factory();
			Gtk::Widget *get_ui_controls();

		protected:

			void in_play_assignment();

			RefPtr<Player> minus_one_assignment();
	};


	class EvenBetterStrategyFactory : public StrategyFactory {
		public:
			EvenBetterStrategyFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	EvenBetterStrategyFactory::EvenBetterStrategyFactory() : StrategyFactory("Even Better than Better Strategy") {
	}

	RefPtr<Strategy2> EvenBetterStrategyFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new EvenBetterStrategy(world));
		return s;
	}

	EvenBetterStrategyFactory factory;

	EvenBetterStrategy::EvenBetterStrategy(RefPtr<World> w) : BasicStrategy(w) {
	}

	Gtk::Widget *EvenBetterStrategy::get_ui_controls() {
		return NULL;
	}

	void EvenBetterStrategy::in_play_assignment() {
		const FriendlyTeam &the_team(world->friendly);
		const Point ballpos = world->ball()->position();

		roles.clear();
		if (the_team.size() == 0) return;

		// const vector<RefPtr<Player> > players = the_team.get_players();

		RefPtr<Defensive2> defensive_role(new Defensive2(world));
		RefPtr<Offensive> offensive_role(new Offensive(world));
		roles.push_back(RefPtr<Role>(defensive_role));
		roles.push_back(RefPtr<Role>(offensive_role));
		std::vector<RefPtr<Player> > defenders;
		std::vector<RefPtr<Player> > offenders;

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
				if (the_team.get_player(i)->chicker_ready_time() >= Player::CHICKER_FOREVER)
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

		std::vector<RefPtr<Player> > rem_players;
		for (size_t i = 0; i < the_team.size(); i++){
			if (static_cast<int>(i) == goalie_index) continue;
			if (static_cast<int>(i) == baller) continue;
			if (baller == -1 && static_cast<int>(i) == baller_ignore_chicker) continue;
			rem_players.push_back(the_team.get_player(i));
		}
		std::sort(rem_players.begin(), rem_players.end(), AIUtil::CmpDist<RefPtr<Player> >(Point(-world->field().length()/2.0,0.0)));

		// preferred_offender_number includes the offender assigned above (closest player to ball)
		// 3 players => 1 offender
		// 4 players => 1 offender
		// 5 players => 2 offenders;
		int preferred_offender_number = std::max(1, static_cast<int>(the_team.size()) - 3);
		switch(world->playtype()){
			case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			case PlayType::PREPARE_KICKOFF_ENEMY:
			case PlayType::EXECUTE_KICKOFF_ENEMY:
			case PlayType::PREPARE_PENALTY_ENEMY:
			case PlayType::EXECUTE_PENALTY_ENEMY:
				preferred_offender_number--;
				if (preferred_offender_number < 1)
					preferred_offender_number = 1;
				break;
			default:
				if (the_team.size() >= 5 && AIUtil::friendly_posses_ball(world))
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

	RefPtr<Player> EvenBetterStrategy::minus_one_assignment() {
		const FriendlyTeam &the_team(world->friendly);
		const Point ballpos = world->ball()->position();

		roles.clear();
		if (the_team.size() == 0) return RefPtr<Player>();

		if (the_team.size() == 1) return the_team.get_player(0);

		// const vector<RefPtr<Player> > players = the_team.get_players();

		RefPtr<Defensive2> defensive_role(new Defensive2(world));
		RefPtr<Offensive> offensive_role(new Offensive(world));
		roles.push_back(RefPtr<Role>(defensive_role));
		roles.push_back(RefPtr<Role>(offensive_role));
		std::vector<RefPtr<Player> > defenders;
		std::vector<RefPtr<Player> > offenders;

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
				if (the_team.get_player(i)->chicker_ready_time() >= Player::CHICKER_FOREVER)
					continue;
				if (ball_dist < best_ball_dist){
					baller = i;
					best_ball_dist = ball_dist;
				}
			}
		}

		std::vector<RefPtr<Player> > rem_players;
		for (size_t i = 0; i < the_team.size(); i++){
			if (static_cast<int>(i) == goalie_index) continue;
			if (static_cast<int>(i) == baller) continue;
			if (baller == -1 && static_cast<int>(i) == baller_ignore_chicker) continue;
			rem_players.push_back(the_team.get_player(i));
		}
		std::sort(rem_players.begin(), rem_players.end(), AIUtil::CmpDist<RefPtr<Player> >(Point(-world->field().length()/2.0,0.0)));

		// preferred_offender_number includes the assigned kicker (closest player to ball)
		// 3 players => 1 offender
		// 4 players => 1 offender
		// 5 players => 2 offenders;
		int preferred_offender_number = std::max(1, static_cast<int>(the_team.size()) - 3);
		switch(world->playtype()){
			case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			case PlayType::PREPARE_KICKOFF_ENEMY:
			case PlayType::EXECUTE_KICKOFF_ENEMY:
			case PlayType::PREPARE_PENALTY_ENEMY:
			case PlayType::EXECUTE_PENALTY_ENEMY:
				preferred_offender_number--;
				if (preferred_offender_number < 1)
					preferred_offender_number = 1;
				break;
			default:
				if (the_team.size() >= 5 && AIUtil::friendly_posses_ball(world))
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

	StrategyFactory &EvenBetterStrategy::get_factory() {
		return factory;
	}

}

