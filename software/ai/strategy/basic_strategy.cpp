#include "ai/strategy/basic_strategy.h"
#include "ai/tactic/chase.h"
#include "ai/role/defensive.h"
#include "ai/role/goalie.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_free_kick.h" 
#include "ai/role/pit_stop.h"   
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/prepare_kickoff_friendly.h"             
#include "ai/role/prepare_penalty_enemy.h"
#include "ai/role/execute_kickoff_friendly.h"             
#include "ai/role/prepare_penalty_friendly.h"
#include "ai/role/execute_penalty_enemy.h"
#include "ai/role/victory_dance.h"
#include "ai/role/execute_penalty_friendly.h"
#include "ai/role/halt.h"
#include "ai/role/stop.h"

#include <iostream>

namespace {

	class basic_strategy_factory : public strategy_factory {
		public:
			basic_strategy_factory();
			strategy::ptr create_strategy(world::ptr world);
	};

	basic_strategy_factory::basic_strategy_factory() : strategy_factory("Basic Strategy") {
	}

	strategy::ptr basic_strategy_factory::create_strategy(world::ptr world) {
		strategy::ptr s(new basic_strategy(world));
		return s;
	}

	basic_strategy_factory factory;
}

basic_strategy::basic_strategy(world::ptr world) : the_world(world) {
	update_wait = 0;
	update_wait_turns = 5;
	the_world->friendly.signal_robot_added.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &basic_strategy::reset_all))));
	the_world->friendly.signal_robot_removed.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &basic_strategy::reset_all))));
	the_world->signal_playtype_changed.connect(sigc::mem_fun(this, &basic_strategy::reset_all));
}

void basic_strategy::tick() {
	update_wait++;
	if (update_wait >= update_wait_turns) {
		update_wait = 0;
		if (the_world->playtype() == playtype::play)
			in_play_assignment();
	}
	for (size_t i = 0; i < roles.size(); i++) {
		roles[i]->tick();
	}
}

Gtk::Widget *basic_strategy::get_ui_controls() {
	return NULL;
}

void basic_strategy::in_play_assignment() {
	const friendly_team &the_team(the_world->friendly);

	roles.clear();
	if (the_team.size() == 0) return;

	goalie::ptr goalie_role(new goalie(the_world));
	roles.push_back(role::ptr(goalie_role));
	std::vector<player::ptr> goalie;
	goalie.push_back(the_team.get_player(0));
	goalie_role->set_robots(goalie);

	defensive::ptr defensive_role(new defensive(the_world));
	offensive::ptr offensive_role(new offensive(the_world));
	roles.push_back(role::ptr(defensive_role));
	roles.push_back(role::ptr(offensive_role));
	std::vector<player::ptr> defenders;
	std::vector<player::ptr> offenders;

	if (the_team.size() >= 2)
		offenders.push_back(the_team.get_player(1));

	if (the_team.size() >= 3)
		defenders.push_back(the_team.get_player(2));

	if (the_team.size() >= 4)
		offenders.push_back(the_team.get_player(3));

	if (the_team.size() >= 5)
		defenders.push_back(the_team.get_player(4));
	
	// extra players become offenders
	for (size_t i = 5; i < the_team.size(); ++i)
		offenders.push_back(the_team.get_player(i));

	defensive_role->set_robots(defenders);
	offensive_role->set_robots(offenders);
}

player::ptr basic_strategy::minus_one_assignment() {
	const friendly_team &the_team(the_world->friendly);

	roles.clear();
	if (the_team.size() == 0) return player::ptr();

	if (the_team.size() == 0) return the_team.get_player(0);

	goalie::ptr goalie_role(new goalie(the_world));
	roles.push_back(role::ptr(goalie_role));
	std::vector<player::ptr> goalie;
	goalie.push_back(the_team.get_player(0));
	goalie_role->set_robots(goalie);

	defensive::ptr defensive_role(new defensive(the_world));
	offensive::ptr offensive_role(new offensive(the_world));
	roles.push_back(role::ptr(defensive_role));
	roles.push_back(role::ptr(offensive_role));
	std::vector<player::ptr> defenders;
	std::vector<player::ptr> offenders;

	if (the_team.size() >= 3)
		defenders.push_back(the_team.get_player(2));

	if (the_team.size() >= 4)
		offenders.push_back(the_team.get_player(3));

	if (the_team.size() >= 5)
		defenders.push_back(the_team.get_player(4));
	
	// extra players become offenders
	for (size_t i = 5; i < the_team.size(); ++i)
		offenders.push_back(the_team.get_player(i));

	defensive_role->set_robots(defenders);
	offensive_role->set_robots(offenders);

	return the_team.get_player(1);
}

void basic_strategy::reset_all() {
	const friendly_team &the_team(the_world->friendly);

	roles.clear();

	if (the_team.size() == 0) return;

	// goalie only
	const std::vector<player::ptr> goalie_only(1, the_team.get_player(0));

	// non goalie
	std::vector<player::ptr> all_players;

	// goalie is the first player
	for (size_t i = 1; i < the_team.size(); i++) {
		all_players.push_back(the_team.get_player(i));
	}

	// shooter for penalty kick
	std::vector<player::ptr> shooter_only;

	if (all_players.size() > 0) {
		shooter_only.push_back(all_players[0]);
	}

	std::vector<player::ptr> non_shooters;
	
	for (size_t i = 1; i < all_players.size(); ++i) {
		non_shooters.push_back(all_players[i]);
	}
	
	#warning For special plays, need logic to choose which robot becomes the kicker
	switch (the_world->playtype()) {
		case playtype::play:
		case playtype::stop:
		case playtype::execute_direct_free_kick_enemy:
		case playtype::execute_indirect_free_kick_enemy:
			in_play_assignment();
			std::cout << all_players.size() << " robots set to play" << std::endl;
			break;

		case playtype::halt:
			roles.push_back(role::ptr(new halt));
			all_players.push_back(goalie_only[0]);
			roles[0]->set_robots(all_players);
			std::cout << all_players.size() << " robots set to halt" << std::endl;
			break;

		case playtype::prepare_kickoff_friendly: 
			roles.push_back(role::ptr(new prepare_kickoff_friendly(the_world)));
			roles[0]->set_robots(all_players);
			std::cout << all_players.size() << " robots set to prepare kickoff friendly" << std::endl;
			break;

		case playtype::execute_kickoff_friendly: 
			roles.push_back(role::ptr(new execute_kickoff_friendly(the_world)));
			roles[0]->set_robots(all_players);
			std::cout << all_players.size() << " robots set to execute kickoff friendly" << std::endl;
			break;

		case playtype::prepare_kickoff_enemy:
			roles.push_back(role::ptr(new prepare_kickoff_enemy(the_world)));
			roles[0]->set_robots(all_players);
			std::cout << all_players.size() << " robots set to prepare kickoff enemy" << std::endl;
			break;

		case playtype::execute_kickoff_enemy:
			roles.push_back(role::ptr(new prepare_kickoff_enemy(the_world)));
			roles[0]->set_robots(all_players);
			std::cout << all_players.size() << " robots set to execute kickoff enemy" << std::endl;
			break;

		case playtype::prepare_penalty_friendly: 
			roles.push_back(role::ptr(new prepare_penalty_friendly(the_world)));
			roles[0]->set_robots(all_players);
			std::cout << all_players.size() << " robots set to prepare penalty friendly" << std::endl;
			break;

		case playtype::execute_penalty_friendly: 
			roles.push_back(role::ptr(new execute_penalty_friendly(the_world)));
			roles[0]->set_robots(shooter_only);

			roles.push_back(role::ptr(new prepare_penalty_friendly(the_world)));
			roles[1]->set_robots(non_shooters);
			
			std::cout << shooter_only.size() << " robots set to execute penalty friendly" << std::endl;
			std::cout << non_shooters.size() << " robots set to prepare penalty friendly" << std::endl;
			break;

		case playtype::execute_penalty_enemy:
			// "Ready" signal is meant to signal kicker may proceed,
			// so no difference on defending side.
			// May need to detect when the play type needs to be changed to play
		case playtype::prepare_penalty_enemy:
			roles.push_back(role::ptr(new prepare_penalty_enemy(the_world)));
			roles[0]->set_robots(all_players);
			roles.push_back(role::ptr(new execute_penalty_enemy(the_world)));
			roles[1]->set_robots(goalie_only);
			std::cout << all_players.size() << " robots set to prepare penalty enemy" << std::endl;
			std::cout << goalie_only.size() << " robots set to execute penalty enemy" << std::endl;
			break;

		case playtype::execute_direct_free_kick_friendly:
			{
				std::vector<player::ptr> freekicker(1, minus_one_assignment());
				//roles.push_back(role::ptr(new execute_direct_free_kick_friendly(the_world)));
				execute_direct_free_kick::ptr freekicker_role = execute_direct_free_kick::ptr(new execute_direct_free_kick(the_world));
				freekicker_role->set_robots(freekicker);
				//roles[0]->set_robots(all_players);
				roles.push_back(freekicker_role);
				std::cout << all_players.size() << " robots set to execute direct free kick friendly" << std::endl;
			}
			break;

		case playtype::execute_indirect_free_kick_friendly:
			{
				std::vector<player::ptr> freekicker(1, minus_one_assignment());
				execute_indirect_free_kick::ptr freekicker_role = execute_indirect_free_kick::ptr(new execute_indirect_free_kick(the_world));
				freekicker_role->set_robots(freekicker);
				roles.push_back(freekicker_role);
				std::cout << all_players.size() << " robots set to execute indirect free kick" << std::endl;
			}
			break;

		case playtype::pit_stop:
			roles.push_back(role::ptr(new pit_stop(the_world)));
			all_players.push_back(goalie_only[0]);
			roles[0]->set_robots(all_players);
			std::cout << all_players.size() << " robots set to pit stop" << std::endl;
			break;		

		case playtype::victory_dance:
			roles.push_back(role::ptr(new victory_dance));
			all_players.push_back(goalie_only[0]);
			roles[0]->set_robots(all_players);
			std::cout << all_players.size() << " robots set to victory dance" << std::endl;
			break;

		default:
			std::cerr << "Unhandled Playtype" << std::endl;
			break;
	}

	// Only assign goalie role for valid play type
	// In these cases the goalie role is the last role in the vector.
	switch (the_world->playtype()) {
		case playtype::halt:
		case playtype::stop:
		case playtype::play:
		case playtype::prepare_penalty_enemy:
		case playtype::execute_penalty_enemy:
		case playtype::pit_stop:
		case playtype::victory_dance:
		case playtype::execute_direct_free_kick_enemy:
		case playtype::execute_indirect_free_kick_enemy:
		case playtype::execute_direct_free_kick_friendly:
		case playtype::execute_indirect_free_kick_friendly:
			break;
		default:
			std::cerr << "Unhandled Playtype" << std::endl;
		case playtype::prepare_kickoff_friendly: 
		case playtype::execute_kickoff_friendly:
		case playtype::prepare_kickoff_enemy:
		case playtype::execute_kickoff_enemy:
		case playtype::prepare_penalty_friendly:
		case playtype::execute_penalty_friendly:
			roles.push_back(role::ptr((new goalie(the_world))));
			roles[roles.size()-1]->set_robots(goalie_only);
			break;
	}
}

strategy_factory &basic_strategy::get_factory() {
	return factory;
}

