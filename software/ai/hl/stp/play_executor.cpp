#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include "ai/hl/stp/ui.h"

#include <cassert>
#include <glibmm.h>
#include <queue>

using AI::HL::STP::PlayExecutor;
using namespace AI::HL::STP;


Player::Ptr AI::HL::STP::HACK::active_player;
Player::Ptr AI::HL::STP::HACK::last_kicked;


namespace {

	BoolParam goalie_lowest("Goalie is lowest index", "STP/Goalie", true);
	IntParam goalie_pattern_index("Goalie pattern index", "STP/Goalie", 0, 0, 11);

	BoolParam high_priority_always("If higher priority play exists, switch", "STP/PlayExecutor", true);

	void on_robot_removing(std::size_t i, World &w) {
		Player::Ptr plr = w.friendly_team().get(i);
		if (plr == HACK::active_player) {
			HACK::active_player.reset(); 
		}
		if (plr == HACK::last_kicked) {
			HACK::last_kicked.reset();
		}
	}

	void connect_player_remove_handler(World &w) {
		static bool connected = false;
		if (!connected) {
			w.friendly_team().signal_robot_removing().connect(sigc::bind(&on_robot_removing, sigc::ref(w)));
			connected = true;
		}
	}

}

PlayExecutor::PlayExecutor(World &w) : world(w) {
	connect_player_remove_handler(w);
	// initialize all plays
	const Play::PlayFactory::Map &m = Play::PlayFactory::all();
	assert(m.size() != 0);
	for (Play::PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
		plays.push_back(i->second->create(world));
	}

	world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &PlayExecutor::on_player_added));
	world.friendly_team().signal_robot_removed().connect(sigc::mem_fun(this, &PlayExecutor::on_player_removed));
}

void PlayExecutor::calc_play() {

	curr_play.reset();

	// find a valid play
	std::random_shuffle(plays.begin(), plays.end());
	for (std::size_t i = 0; i < plays.size(); ++i) {
		if (!plays[i]->factory().enable) continue;
		if (!plays[i]->invariant()) continue;
		if (!plays[i]->applicable()) continue;
		if (plays[i]->done()) {
			LOG_ERROR(Glib::ustring::compose("Play applicable but done: %1", plays[i]->factory().name()));
			continue;
		}

		LOG_DEBUG(Glib::ustring::compose("Play candidate: %1", plays[i]->factory().name()));

		if (!curr_play.is() || plays[i]->factory().priority > curr_play->factory().priority) {
			curr_play = plays[i];
		}
	}

	if (!curr_play.is()) {
		return;
	}

	LOG_INFO(Glib::ustring::compose("Play chosen: %1", curr_play->factory().name()));

	curr_role_step = 0;
	for (std::size_t j = 0; j < 5; ++j) {
		curr_roles[j].clear();
		// default to idle tactic
		curr_tactic[j] = Tactic::idle(world);
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
	return;
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

	std::fill(curr_assignment, curr_assignment + 5, Player::Ptr());

	Player::Ptr goalie;
	if (goalie_lowest) {
		goalie = world.friendly_team().get(0);
	} else {
		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			Player::Ptr p = world.friendly_team().get(i);
			if (p->pattern() == goalie_pattern_index) {
				goalie = p;
			}
		}
	}

	if (!goalie.is()) {
		LOG_ERROR("No goalie with the desired pattern");
		curr_play.reset();
		return;
	}

	assert(curr_tactic[0].is());
	curr_tactic[0]->set_player(goalie);
	curr_assignment[0] = goalie;

	// pool of available people
	std::set<Player::Ptr> players;
	for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
		Player::Ptr p = world.friendly_team().get(i);
		if (p == goalie) continue;
		players.insert(p);
	}

	bool active_assigned = (curr_tactic[0]->active());
	for (std::size_t i = 1; i < 5 && players.size() > 0; ++i) {
		curr_assignment[i] = curr_tactic[i]->select(players);
		// assignment cannot be empty
		assert(curr_assignment[i].is());
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
		curr_play.reset();
		return;
	}
}

void PlayExecutor::execute_tactics() {
	std::size_t max_role_step = 0;
	for (std::size_t i = 0; i < 5; ++i) {
		max_role_step = std::max(max_role_step, curr_roles[i].size());
	}

	while (true) {
		role_assignment();

		// if role assignment failed
		if (!curr_play.is()) {
			return;
		}

		if (curr_active->fail()) {
			LOG_INFO(Glib::ustring::compose("%1: active tactic failed", curr_play->factory().name()));
			curr_play.reset();
			return;
		}

		// it is possible to skip steps
		if (curr_active->done()) {
			++curr_role_step;

			// when the play runs out of tactics, they are done!
			if (curr_role_step >= max_role_step) {
				LOG_INFO(Glib::ustring::compose("%1: all tactics done", curr_play->factory().name()));
				curr_play.reset();
				return;
			}

			continue;
		}

		break;
	}

	// set flags, do it before any execution
	curr_assignment[0]->flags(0);
	for (std::size_t i = 1; i < 5; ++i) {
		if (!curr_assignment[i].is()) {
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
				default_flags |= Flags::FLAG_FRIENDLY_KICK;
				break;
			case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
			case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
				default_flags |= Flags::FLAG_AVOID_BALL_STOP;
				default_flags |= Flags::FLAG_STAY_OWN_HALF;
				break;
			default:
				break;
		}
		curr_assignment[i]->flags(default_flags);
	}

	// execute!
	for (std::size_t i = 0; i < 5; ++i) {
		if (!curr_assignment[i].is()) {
			continue;
		}
		curr_tactic[i]->execute();
	}

	if (curr_active->fail()) {
		LOG_INFO(Glib::ustring::compose("%1: active tactic failed", curr_play->factory().name()));
		curr_play.reset();
		return;
	}

	// an active tactic may be done
	if (curr_active->done()) {
		++curr_role_step;

		// when the play runs out of tactics, they are done!
		if (curr_role_step >= max_role_step) {
			LOG_INFO(Glib::ustring::compose("%1: all tactics done", curr_play->factory().name()));
			curr_play.reset();
			return;
		}
	}
}

void PlayExecutor::tick() {
	Evaluation::tick_offense(world);

	// override halt completely
	if (world.friendly_team().size() == 0 || world.playtype() == AI::Common::PlayType::HALT) {
		curr_play.reset();
		return;
	}

	// check if curr play wants to continue
	if (curr_play.is()) {
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
				if (!plays[i]->factory().enable) continue;
				if (!plays[i]->invariant()) continue;
				if (!plays[i]->applicable()) continue;
				if (plays[i]->factory().priority > curr_play->factory().priority) {
					LOG_INFO("higher priority play exist");
					done = true;
					break;
				}
			}
		}

		if (done) {
			LOG_INFO(Glib::ustring::compose("%1: play tactic failed", curr_play->factory().name()));
			curr_play.reset();
		}
	}

	// check if curr is valid
	if (!curr_play.is()) {
		calc_play();
		if (!curr_play.is()) {
			LOG_ERROR("calc play failed");
			return;
		}
	}

	execute_tactics();
}

std::string PlayExecutor::info() const {
	std::ostringstream text;
	if (curr_play.is()) {
		text << "play: " << curr_play->factory().name();
		text << std::endl;
		text << "step: " << curr_role_step;
		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			text << std::endl;
			text << curr_assignment[i]->pattern() << ": ";
			if (curr_tactic[i]->active()) {
				text << "*";
			} else {
				text << " ";
			}
			text << curr_tactic[i]->description();
		}
	} else {
		text << "No Play";
	}
	return text.str();
}

void PlayExecutor::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
	draw_shoot(world, ctx);
	draw_offense(world, ctx);
	draw_defense(world, ctx);
	draw_enemy_pass(world, ctx);
	draw_friendly_pass(world, ctx);
	draw_player_status(world, ctx);
	// draw_velocity(ctx); // uncommand to display velocity
	if (world.playtype() == AI::Common::PlayType::STOP) {
		ctx->set_source_rgb(1.0, 0.5, 0.5);
		ctx->arc(world.ball().position().x, world.ball().position().y, 0.5, 0.0, 2 * M_PI);
		ctx->stroke();
	}
	if (!curr_play.is()) {
		return;
	}
	curr_play->draw_overlay(ctx);
	for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
		const std::vector<Tactic::Tactic::Ptr> &role = curr_roles[i];
		for (std::size_t t = 0; t < role.size(); ++t) {
			role[t]->draw_overlay(ctx);
		}
	}
}

void PlayExecutor::on_player_added(std::size_t) {
	LOG_INFO("Player added");
}

void PlayExecutor::on_player_removed() {
	LOG_INFO("Player removed, reset play");
	curr_play.reset();
}

