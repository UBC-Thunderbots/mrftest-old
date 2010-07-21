#include "ai/strategy/basic2_strategy.h"
#include "ai/util.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_free_kick.h" 
#include "ai/role/pit_stop.h"   
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/kickoff_friendly.h"             
#include "ai/role/penalty_friendly.h"
#include "ai/role/penalty_enemy.h"
#include "ai/role/victory_dance.h"
#include "util/dprint.h"

#include <iostream>

namespace {

	class Basic2Factory : public StrategyFactory {
		public:
			Basic2Factory();
			Strategy2::Ptr create_strategy(World::Ptr world);
	};

	Basic2Factory::Basic2Factory() : StrategyFactory("Basic V2") {
	}

	Strategy2::Ptr Basic2Factory::create_strategy(World::Ptr world) {
		Strategy2::Ptr s(new Basic2(world));
		return s;
	}

	Basic2Factory factory;

	/// playtypes require a player from friendly team to do something 
	bool active_playtype(const PlayType::PlayType& playtype) {
		switch (playtype) {
			case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			case PlayType::PREPARE_KICKOFF_FRIENDLY: 
			case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			case PlayType::PREPARE_PENALTY_FRIENDLY:
			case PlayType::EXECUTE_PENALTY_FRIENDLY:
				return true;
			default:
				break;
		}
		return false;
	}

}

Basic2::Basic2(World::Ptr world) : world(world), defensive(world), offensive(world) {
	world->friendly.signal_player_added.connect(sigc::mem_fun(this, &Basic2::player_added));
	world->friendly.signal_player_removed.connect(sigc::mem_fun(this, &Basic2::player_removed));
	world->signal_playtype_changed.connect(sigc::mem_fun(this, &Basic2::playtype_changed));
	prev_playtype = world->playtype();
}

void Basic2::player_added(unsigned int index, Player::Ptr player) {
	const FriendlyTeam &friendly(world->friendly);
	if (!goalie) {
		if (friendly.size() != 1) {
			LOG_ERROR("goalie assignment problem");
		}
		goalie = player;
	}
#warning TODO reassign roles
}

void Basic2::player_removed(unsigned int index, Player::Ptr player) {
	// the use of this is justified
	reset_assignments();
}

void Basic2::reset_assignments() {
	const FriendlyTeam &friendly(world->friendly);
	goalie.clear();
	executor.clear();
	if (friendly.size() == 0) return;
	if (active_playtype(world->playtype())) {
		/// assign an executor player
		/// if penalty, use the goalie
	}
}

int Basic2::calc_num_offenders() const {
	const FriendlyTeam &friendly(world->friendly);
	const Point ballpos = world->ball()->position();

	if (friendly.size() == 0) return 0;

	if (friendly.size() == 1) {
		switch (world->playtype()) {
			case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			case PlayType::PREPARE_KICKOFF_FRIENDLY:
			case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			case PlayType::PREPARE_PENALTY_FRIENDLY:
			case PlayType::EXECUTE_PENALTY_FRIENDLY:
				return 1;
			default:
				break;
		}
		// want goalie to defend
		return 0;
	}

	// TODO: change the following
	// For now, goalie is always lowest numbered robot
	int goalie_index = 0;

	int baller = -1;
	int baller_ignore_chicker = -1;
	if (friendly.size() >= 2) {
		double best_ball_dist = 1e50;
		double best_ball_dist_ignore_chicker = 1e50;
		for (size_t i = 0; i < friendly.size(); i++){
			if (static_cast<int>(i) == goalie_index)
				continue;
			double ball_dist = (ballpos - friendly[i]->position()).len();
			if (ball_dist < best_ball_dist_ignore_chicker){
				baller_ignore_chicker = i;
				best_ball_dist_ignore_chicker = ball_dist;
			}
			if (friendly[i]->chicker_ready_time() >= Player::CHICKER_FOREVER)
				continue;
			if (ball_dist < best_ball_dist){
				baller = i;
				best_ball_dist = ball_dist;
			}
		}
	}

	std::vector<Player::Ptr > rem_players;
	for (size_t i = 0; i < friendly.size(); i++){
		if (static_cast<int>(i) == goalie_index) continue;
		if (static_cast<int>(i) == baller) continue;
		if (baller == -1 && static_cast<int>(i) == baller_ignore_chicker) continue;
		rem_players.push_back(friendly[i]);
	}
	std::sort(rem_players.begin(), rem_players.end(), AIUtil::CmpDist<Player::Ptr >(Point(-world->field().length()/2.0,0.0)));

	// preferred_offender_number includes the assigned kicker (closest player to ball)
	// 3 players => 1 offender
	// 4 players => 1 offender
	// 5 players => 2 offenders;
	int preferred_offender_number = std::max(1, static_cast<int>(friendly.size()) - 3);
	switch (world->playtype()){
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
			if (friendly.size() >= 5 && AIUtil::friendly_posses_ball(world))
				preferred_offender_number++;
	}

	// preferred_defender_number includes goalie
	// int preferred_defender_number = friendly.size() - preferred_offender_number;
	return preferred_offender_number;
}

void Basic2::playtype_changed() {
	const FriendlyTeam &friendly(world->friendly);
	switch (world->playtype()) {
		case PlayType::STOP:
		case PlayType::PIT_STOP:
		case PlayType::VICTORY_DANCE:
		case PlayType::HALT:
			reset_assignments();
			break;
		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
		case PlayType::PLAY:
			if (executor) {
				// place this person in offense
				// offensive.add_player(executor);
				executor.clear();
			}
			break;
		case PlayType::EXECUTE_KICKOFF_ENEMY:
		case PlayType::PREPARE_KICKOFF_ENEMY:
			// formation assignment
			break;
		case PlayType::PREPARE_KICKOFF_FRIENDLY: 
		case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			// executor assignment
			break;
		case PlayType::PREPARE_PENALTY_FRIENDLY:
		case PlayType::EXECUTE_PENALTY_FRIENDLY:
			// executor assignment
			break;
		case PlayType::EXECUTE_PENALTY_ENEMY:
		case PlayType::PREPARE_PENALTY_ENEMY:
			// goalie = executor
			break;
		case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
		case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			// executor assignment
			break;
		default:
			LOG_ERROR("unhandled playtype");
			break;
	}
	prev_playtype = world->playtype();
}

void Basic2::tick() {
#warning TODO fix
	defensive.tick();
	offensive.tick();
}

Gtk::Widget *Basic2::get_ui_controls() {
	return NULL;
}

StrategyFactory &Basic2::get_factory() {
	return factory;
}

