#include "ai/hl/stp/play_executor.h"
#include "ai/hl/util.h"
#include "util/dprint.h"

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

PlayExecutor::PlayExecutor(AI::HL::W::World& w) : world(w), initialized(false) {
	world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &PlayExecutor::on_player_added));
	world.friendly_team().signal_robot_removed().connect(sigc::mem_fun(this, &PlayExecutor::on_player_removed));
}

void PlayExecutor::reset() {
	curr_play.reset();
}

void PlayExecutor::initialize() {
	if (initialized) return;
	const PlayFactory::Map &m = PlayFactory::all();
	for (PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
		plays.push_back(i->second->create(world));
	}
	initialized = true;
}

void PlayExecutor::calc_play() {

	if (!initialized) {
		initialize();
	}

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
			// initialize this play and run it
			curr_play->assign(curr_roles[0], curr_roles + 1);
			LOG_INFO(curr_play->factory().name());
			return;
		}
	}
}

void PlayExecutor::role_assignment() {
	std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

	// this must be reset every tick
	curr_active.reset();

	for (std::size_t i = 0; i < 5; ++i) {

		if (curr_role_step < curr_roles[i].size()) {
			curr_tactic[i] = curr_roles[i][curr_role_step];
		} else {
			// if there are no more tactics, use the previous one
			// BUT active tactic cannot be reused!
			if (curr_tactic[i]->active()) {
				LOG_ERROR("Cannot re-use active tactic!");
				reset();
				return;
			}
		}

		if (curr_tactic[i]->active()) {
			// we cannot have more than 1 active tactic.
			if (curr_active.is()) {
				LOG_ERROR("Multiple active tactics");
				reset();
				return;
			}
			curr_active = curr_tactic[i];
		}
	}

	// we cannot have less than 1 active tactic.
	if (!curr_active.is()) {
		LOG_ERROR("No active tactic");
		reset();
		return;
	}

	// do matching
	bool players_used[5];

	std::fill(players_used, players_used + 5, false);
	for (std::size_t i = 0; i < 5; ++i) {
		curr_assignment[i].reset();
	}

	for (std::size_t i = 0; i < 5; ++i) {
		if (!curr_tactic[i].is()) {
			break;
		}

		if (i == 0) { // only for goalie
			players_used[0] = true;
			curr_assignment[0] = players[0];
		} else { // others players
			double best_score = 0;
			std::size_t best_j = 0;
			Player::Ptr best;
			for (std::size_t j = 0; j < players.size(); ++j) {
				if (players_used[j]) {
					continue;
				}
				double score = curr_tactic[i]->score(players[j]);
				if (!best.is() || score > best_score) {
					best = players[j];
					best_j = j;
					best_score = score;
				}
			}
			if (best.is()) {
				players_used[best_j] = true;
				curr_assignment[i] = best;
			}
		}
	}

	bool active_assigned = false;
	for (std::size_t i = 0; i < 5; ++i) {
		if (!curr_assignment[i].is()) {
			continue;
		}
		curr_tactic[i]->set_player(curr_assignment[i]);
		if (curr_tactic[i]->active()) {
			active_assigned = true;
		}
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

	// update the evaluation modules
	curr_play->tick();

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

