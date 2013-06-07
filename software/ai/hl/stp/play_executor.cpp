#include "ai/hl/stp/play_executor.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/tactic/idle.h"
#include "util/dprint.h"
#include <cassert>
#include <utility>
#include <glibmm/ustring.h>

using AI::HL::STP::PlayExecutor;
using namespace AI::HL::STP;

Player AI::HL::STP::HACK::active_player;
Player AI::HL::STP::HACK::last_kicked;

namespace AI {
	namespace HL {
		namespace STP {
			Player _goalie;

			size_t team_size = 0;
		}
	}
}

namespace {
	// One of the changes from the technical committee this year (2013) is that the
	// referee box will send the pattern number of the goalie for each team.
	// You can retrieve this with the goalie() function on a Team object.
	// Therefore use the following goalie code only for testing. 
#ifdef TEST_GOALIE
	BoolParam goalie_lowest("Goalie is lowest index", "STP/Goalie", true);
	IntParam goalie_pattern_index("Goalie pattern index", "STP/Goalie", 0, 0, 11);
#endif
	BoolParam high_priority_always("If higher priority play exists, switch", "STP/PlayExecutor", true);
	IntParam playbook_index("Current Playbook, use bitwise operations", "STP/PlayExecutor", 0, 0, 9);
}

PlayExecutor::PlayExecutor(World w) : world(w), curr_play(0), curr_active(0) {
	std::fill(curr_tactic, curr_tactic + TEAM_MAX_SIZE, static_cast<Tactic::Tactic *>(0));

	for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
		idle_tactics[i] = Tactic::idle(world);
	}

	// initialize all plays
	const Play::PlayFactory::Map &m = Play::PlayFactory::all();
	assert(m.size() != 0);
	for (Play::PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
		plays.push_back(i->second->create(world));
	}

	world.friendly_team().signal_membership_changed().connect(sigc::mem_fun(this, &PlayExecutor::clear_assignments));
}

void PlayExecutor::calc_play() {
	curr_play = 0;

	// find a valid play
	std::random_shuffle(plays.begin(), plays.end());
	for (std::size_t i = 0; i < plays.size(); ++i) {
		if (!(plays[i]->factory().playbook & (1 << playbook_index))) {
			continue;
		}
		if (!plays[i]->factory().enable) {
			continue;
		}
		if (!plays[i]->invariant()) {
			continue;
		}
		if (!plays[i]->applicable()) {
			continue;
		}
		if (plays[i]->done()) {
			LOG_ERROR(Glib::ustring::compose("Play applicable but done: %1", plays[i]->factory().name()));
			continue;
		}

		LOG_DEBUG(Glib::ustring::compose("Play candidate: %1", plays[i]->factory().name()));

		if (!curr_play || plays[i]->factory().priority > curr_play->factory().priority) {
			curr_play = plays[i].get();
		}
	}

	if (!curr_play) {
		return;
	}

	LOG_INFO(Glib::ustring::compose("Play chosen: %1", curr_play->factory().name()));

	curr_role_step = 0;
	for (std::size_t j = 0; j < TEAM_MAX_SIZE; ++j) {
		curr_roles[j].clear();
		// default to idle tactic
		curr_tactic[j] = idle_tactics[j].get();
	}
	// assign the players
	{
		std::vector<Tactic::Tactic::Ptr> goalie_role;
		std::vector<Tactic::Tactic::Ptr> normal_roles[TEAM_MAX_SIZE-1]; // minus goalie
		curr_play->assign(goalie_role, normal_roles);
		std::swap(goalie_role, curr_roles[0]);
		for (std::size_t j = 1; j < TEAM_MAX_SIZE; ++j) {
			std::swap(normal_roles[j - 1], curr_roles[j]);
		}
	}
}

void PlayExecutor::role_assignment() {
	// this must be reset every tick
	curr_active = 0;

	for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
		if (curr_role_step < curr_roles[i].size()) {
			curr_tactic[i] = curr_roles[i][curr_role_step].get();
		} else {
			// if there are no more tactics, use the previous one
			// BUT active tactic cannot be reused!
			assert(!curr_tactic[i]->active());
		}

		if (curr_tactic[i]->active()) {
			// we cannot have more than 1 active tactic.
			assert(!curr_active);
			curr_active = curr_tactic[i];
		}
	}

	// we cannot have less than 1 active tactic.
	assert(curr_active);

	std::fill(curr_assignment, curr_assignment + TEAM_MAX_SIZE, Player());

	
	Player goalie;
	#warning This removes the safety check of must having a goalie to execute a play. 
	for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
		Player p = world.friendly_team().get(i);
		if (p.pattern() == world.friendly_team().goalie()) {
			goalie = p;
		}
	}
#ifdef TEST_GOALIE
	if (goalie_lowest) {
		goalie = world.friendly_team().get(0);
	} else {
		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			Player p = world.friendly_team().get(i);
			if (p.pattern() == static_cast<unsigned int>(goalie_pattern_index)) {
				goalie = p;
			}
		}
	} 

	if (!goalie) {
		LOG_ERROR("No goalie with the desired pattern");
		curr_play = 0;
		return;
	}
#endif
	if (goalie) {
		AI::HL::STP::_goalie = goalie;

		assert(curr_tactic[0]);
		curr_tactic[0]->set_player(goalie);
		curr_assignment[0] = goalie;
	} else {
		LOG_ERROR("No goalie with the desired pattern");
	}
	// pool of available people
	std::set<Player> players;
	for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
		Player p = world.friendly_team().get(i);
		if (goalie && p == goalie) {
			continue;
		}
		players.insert(p);
	}

	team_size = 1 + players.size();

	bool active_assigned = false; 
	if (goalie) {
		active_assigned = (curr_tactic[0]->active());
	}
	for (std::size_t i = 1; i < TEAM_MAX_SIZE; ++i) {
		if (players.empty()) {
			break;
		}
		curr_assignment[i] = curr_tactic[i]->select(players);
		// assignment cannot be empty
		assert(curr_assignment[i]);
		assert(players.find(curr_assignment[i]) != players.end());
		players.erase(curr_assignment[i]);
		curr_tactic[i]->set_player(curr_assignment[i]);
		if (curr_tactic[i]->active()) {
			active_assigned = true;
			HACK::active_player = curr_assignment[i];
		}
	}

	// can't assign active tactic to anyone
	if (!active_assigned) {
		LOG_ERROR("Active tactic not assigned");
		curr_play = 0;
		return;
	}
}

void PlayExecutor::execute_tactics() {
	std::size_t max_role_step = 0;
	for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
		max_role_step = std::max(max_role_step, curr_roles[i].size());
	}

	while (true) {
		role_assignment();

		// if role assignment failed
		if (!curr_play) {
			return;
		}

		if (curr_active->fail()) {
			LOG_INFO(Glib::ustring::compose("%1: active tactic failed", curr_play->factory().name()));
			curr_play = 0;
			return;
		}

		// it is possible to skip steps
		if (curr_active->done()) {
			++curr_role_step;

			// when the play runs out of tactics, they are done!
			if (curr_role_step >= max_role_step) {
				LOG_INFO(Glib::ustring::compose("%1: all tactics done", curr_play->factory().name()));
				curr_play = 0;
				return;
			}

			continue;
		}

		break;
	}

	// set flags, do it before any execution
	if (AI::HL::STP::_goalie){
		curr_assignment[0].flags(0);
	}
	for (std::size_t i = 1; i < TEAM_MAX_SIZE; ++i) {
		if (!curr_assignment[i]) {
			continue;
		}
		unsigned int default_flags = Flags::FLAG_AVOID_FRIENDLY_DEFENSE;
		switch (world.playtype()) {
			case AI::Common::PlayType::STOP:
			case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
				default_flags |= Flags::FLAG_AVOID_BALL_STOP;
				break;

			case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
				default_flags |= Flags::FLAG_AVOID_ENEMY_DEFENSE;
				break;

			case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
			case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
				default_flags |= Flags::FLAG_AVOID_BALL_STOP;
				default_flags |= Flags::FLAG_STAY_OWN_HALF;
				break;

			default:
				break;
		}
		curr_assignment[i].flags(default_flags);
	}

	// execute!
	for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
		if (!curr_assignment[i]) {
			continue;
		}
		curr_tactic[i]->execute();
	}

	if (curr_active->fail()) {
		LOG_INFO(Glib::ustring::compose("%1: active tactic failed", curr_play->factory().name()));
		curr_play = 0;
		return;
	}

	// an active tactic may be done
	if (curr_active->done()) {
		++curr_role_step;

		// when the play runs out of tactics, they are done!
		if (curr_role_step >= max_role_step) {
			LOG_INFO(Glib::ustring::compose("%1: all tactics done", curr_play->factory().name()));
			curr_play = 0;
			return;
		}
	}
}

void PlayExecutor::tick() {
	tick_eval(world);

	// override halt completely
	if (world.friendly_team().size() == 0 || world.playtype() == AI::Common::PlayType::HALT) {
		curr_play = 0;
		return;
	}

	// check if curr play wants to continue
	if (curr_play) {
		bool done = false;
		if (!curr_play->invariant()) {
			LOG_INFO("play invariant no longer holds");
			done = true;
		}
		if (curr_play->done()) {
			LOG_INFO("play done is true");
			done = true;
		}
		if (curr_play->fail()) {
			LOG_INFO("play failed");
			done = true;
		}
		if (high_priority_always && curr_play->can_give_up_safely()) {
			for (std::size_t i = 0; i < plays.size(); ++i) {
				if (!plays[i]->factory().enable) {
					continue;
				}
				if (!plays[i]->invariant()) {
					continue;
				}
				if (!plays[i]->applicable()) {
					continue;
				}
				if (plays[i]->factory().priority > curr_play->factory().priority) {
					LOG_INFO("higher priority play exist");
					done = true;
					break;
				}
			}
		}

		if (done) {
			LOG_INFO(Glib::ustring::compose("%1: play tactic failed", curr_play->factory().name()));
			curr_play = 0;
		}
	}

	// check if curr is valid
	if (!curr_play) {
		calc_play();
		if (!curr_play) {
			LOG_ERROR("calc play failed");
			return;
		}
	}

	execute_tactics();
}

Glib::ustring PlayExecutor::info() const {
	Glib::ustring text;
	if (curr_play) {
		text += Glib::ustring::compose("play: %1\nstep: %2", curr_play->factory().name(), curr_role_step);
		// std::size_t imax = std::min((std::size_t)5, world.friendly_team().size());
		for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
			if (!curr_assignment[i]) {
				// LOG_ERROR("curr-assignment empty");
				continue;
			}
			text += Glib::ustring::compose("\n%1: %2%3", curr_assignment[i].pattern(), curr_tactic[i]->active() ? '*' : ' ', curr_tactic[i]->description());
		}
	} else {
		text = "No Play";
	}
	return text;
}

void PlayExecutor::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
	draw_ui(world, ctx);

	if (world.playtype() == AI::Common::PlayType::STOP) {
		ctx->set_source_rgb(1.0, 0.5, 0.5);
		ctx->arc(world.ball().position().x, world.ball().position().y, 0.5, 0.0, 2 * M_PI);
		ctx->stroke();
	}
	if (!curr_play) {
		return;
	}
	curr_play->draw_overlay(ctx);
	for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
		if (!curr_assignment[i]) {
			continue;
		}
		const std::vector<Tactic::Tactic::Ptr> &role = curr_roles[i];
		for (std::size_t t = 0; t < role.size(); ++t) {
			role[t]->draw_overlay(ctx);
		}
	}
}

void PlayExecutor::clear_assignments() {
	LOG_INFO("Team membership changed, reset play");

	_goalie = AI::HL::W::Player();

	curr_play = 0;
	curr_active = 0;
	for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
		curr_tactic[i] = 0;
		curr_assignment[i] = AI::HL::W::Player();
		curr_roles[i].clear();
	}
}

