#include "ai/hl/stp/play_executor.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <cassert>

using AI::HL::STP::PlayExecutor;
using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayFactory;
using AI::HL::STP::Tactic::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::STP;

namespace {
	// The maximum amount of time a play can be running.
	const double PLAY_TIMEOUT = 30.0;
}

PlayExecutor::PlayExecutor(AI::HL::W::World &w) : world(w) {
	world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &PlayExecutor::on_player_added));
	world.friendly_team().signal_robot_removed().connect(sigc::mem_fun(this, &PlayExecutor::on_player_removed));

	// initialize all plays
	const PlayFactory::Map &m = PlayFactory::all();
	for (PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
		plays.push_back(i->second->create(world));
	}
}

void PlayExecutor::reset() {
	curr_play.reset();
}

void PlayExecutor::calc_play() {
	// find a valid play
	std::random_shuffle(plays.begin(), plays.end());
	for (std::size_t i = 0; i < plays.size(); ++i) {
		if (plays[i]->applicable()) {
			curr_play = plays[i];
			curr_role_step = 0;
			for (std::size_t j = 0; j < 5; ++j) {
				curr_roles[j].clear();
				curr_tactic[j].reset();
			}
			// assign the players
			{
				std::vector<Tactic::Tactic::Ptr> goalie_role;
				std::vector<Tactic::Tactic::Ptr> normal_roles[4];
				curr_play->assign(goalie_role, normal_roles);
				swap(goalie_role, curr_roles[0]);
				for (std::size_t j = 1; j < 5; ++j) {
					swap(normal_roles[j - 1], curr_roles[j]);
				}
			}
			LOG_INFO(curr_play->factory().name());
			return;
		}
	}
}

void PlayExecutor::role_assignment() {
	// this must be reset every tick
	curr_active.reset();

	for (std::size_t i = 0; i < 5; ++i) {
		if (curr_role_step < curr_roles[i].size()) {
			curr_tactic[i] = curr_roles[i][curr_role_step];
		} else {
			// if there are no more tactics, use the previous one
			// BUT active tactic cannot be reused!
			assert(!curr_tactic[i]->active());
		}

		if (curr_tactic[i]->active()) {
			// we cannot have more than 1 active tactic.
			assert(!curr_active.is());
			curr_active = curr_tactic[i];
		}
	}

	// we cannot have less than 1 active tactic.
	assert(curr_active.is());

	std::fill(curr_assignment, curr_assignment + 5, AI::HL::W::Player::Ptr());

	assert(curr_tactic[0].is());
	curr_tactic[0]->set_player(world.friendly_team().get(0));
	curr_assignment[0] = world.friendly_team().get(0);

	// pool of available people
	std::set<Player::Ptr> players;
	for (std::size_t i = 1; i < world.friendly_team().size(); ++i) {
		players.insert(world.friendly_team().get(i));
	}

	bool active_assigned = (curr_tactic[0]->active());
	for (std::size_t i = 1; i < 5 && players.size() > 0; ++i) {
		curr_assignment[i] = curr_tactic[i]->select(players);
		// assignment cannot be empty
		assert(curr_assignment[i].is());
		assert(players.find(curr_assignment[i]) != players.end());
		players.erase(curr_assignment[i]);
		curr_tactic[i]->set_player(curr_assignment[i]);
		active_assigned = active_assigned || curr_tactic[i]->active();
	}

	// can't assign active tactic to anyone
	if (!active_assigned) {
		LOG_ERROR("Active tactic not assigned");
		reset();
		return;
	}
}

void PlayExecutor::execute_tactics() {
	std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

	while (true) {
		role_assignment();

		// if role assignment failed
		if (!curr_play.is()) {
			return;
		}

		// it is possible to skip steps
		if (curr_active->done()) {
			++curr_role_step;
			continue;
		}

		break;
	}

	// execute!
	for (std::size_t i = 0; i < 5; ++i) {
		if (!curr_assignment[i].is()) {
			continue;
		}
		curr_tactic[i]->tick();
	}
}

void PlayExecutor::tick() {
	// check if curr play wants to continue
	if (curr_play.is() && (curr_play->done() || curr_play->fail())) {
		curr_play.reset();
	}

	// check if curr is valid
	if (!curr_play.is()) {
		calc_play();
		if (!curr_play.is()) {
			LOG_WARN("calc play failed");
			return;
		}
	}

	execute_tactics();
}

void PlayExecutor::on_player_added(std::size_t) {
	reset();
}

void PlayExecutor::on_player_removed() {
	reset();
}

