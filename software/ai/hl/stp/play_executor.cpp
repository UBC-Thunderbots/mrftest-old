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

namespace AI {
	namespace HL {
		namespace STP {
			Player _goalie;
		}
	}
}

namespace {
	BoolParam enable0(u8"enable robot 0", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable1(u8"enable robot 1", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable2(u8"enable robot 2", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable3(u8"enable robot 3", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable4(u8"enable robot 4", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable5(u8"enable robot 5", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable6(u8"enable robot 6", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable7(u8"enable robot 7", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable8(u8"enable robot 8", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable9(u8"enable robot 9", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable10(u8"enable robot 10", u8"AI/HL/STP/PlayExecutor", true);
	BoolParam enable11(u8"enable robot 11", u8"AI/HL/STP/PlayExecutor", true);

	BoolParam high_priority_always(u8"If higher priority play exists, switch", u8"AI/HL/STP/PlayExecutor", true);
	IntParam playbook_index(u8"Current Playbook, use bitwise operations", u8"AI/HL/STP/PlayExecutor", 0, 0, 9);
}

PlayExecutor::PlayExecutor(World w) : world(w), curr_play(nullptr) {
	// initialize all plays
	const Play::PlayFactory::Map &m = Play::PlayFactory::all();
	assert(m.size() != 0);

	for (const auto &i : m) {
		plays.push_back(i.second);
	}

	world.friendly_team().signal_membership_changed().connect(sigc::mem_fun(this, &PlayExecutor::clear_assignments));
}

PlayExecutor::~PlayExecutor() {
	stop_threads();
}

void PlayExecutor::calc_play() {
	curr_play = nullptr;

	// find a valid play
	std::random_shuffle(plays.begin(), plays.end());
	for (auto &i : plays) {
		if (!(i->playbook & (1 << playbook_index))) {
			continue;
		}
		if (!i->enable) {
			continue;
		}
		if (!i->invariant(world)) {
			continue;
		}
		if (!i->applicable(world)) {
			continue;
		}

		if (!curr_play || i->priority > curr_play->factory().priority) {
			curr_play = i->create(world);
		}
	}

	if (!curr_play) {
		return;
	}

	LOG_INFO(Glib::ustring::compose(u8"Play chosen: %1", curr_play->factory().name()));
}

void PlayExecutor::tick() {
	tick_eval(world);

	enable_players();

	std::vector<Player> players;
	for (const Player i : world.friendly_team()) {
		if (players_enabled[i.pattern()]) {
			players.push_back(i);
		}
	}

	// override halt completely
	if (players.empty() || world.playtype() == AI::Common::PlayType::HALT) {
		curr_play = nullptr;
		return;
	}

	// check if curr play wants to continue
	if (curr_play) {
		bool done = false;
		if (!curr_play->factory().invariant(world)) {
			LOG_INFO(u8"play invariant no longer holds");
			done = true;
		}
		if (curr_play->done()) {
			LOG_INFO(u8"play done is true");
			done = true;
		}
		if (curr_play->coroutine_finished()) {
			LOG_INFO(u8"play out of tactics");
			done = true;
		}
		if (curr_play->fail()) {
			LOG_INFO(u8"play failed");
			done = true;
		}
		if (high_priority_always && curr_play->can_give_up_safely()) {
			for (auto &i : plays) {
				if (!i->enable) {
					continue;
				}
				if (!i->invariant(world)) {
					continue;
				}
				if (!i->applicable(world)) {
					continue;
				}
				if (i->priority > curr_play->factory().priority) {
					LOG_INFO(u8"higher priority play exists");
					done = true;
					break;
				}
			}
		}

		if (done) {
			LOG_INFO(Glib::ustring::compose(u8"%1: play tactic failed", curr_play->factory().name()));
			curr_play = nullptr;
		}
	}

	// check if curr is valid
	if (!curr_play) {
		calc_play();
		if (!curr_play) {
			LOG_ERROR(u8"calc play failed");
			return;
		}
	} else {
		curr_play->tick(players_enabled);
	}
}

Glib::ustring PlayExecutor::info() const {
	Glib::ustring text;
	if (curr_play) {
		text += Glib::ustring::compose(u8"play: %1", curr_play->factory().name());
		for (std::size_t i = 0; i < TEAM_MAX_SIZE; ++i) {
			if (!curr_play->get_assignment()[i]) {
				continue;
			}
			text += Glib::ustring::compose(u8"\n%1: %2", curr_play->get_assignment()[i].pattern(), curr_play->get_tactics()[i]->description());
		}
	} else {
		text = u8"No Play";
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
}

void PlayExecutor::clear_assignments() {
	LOG_INFO(u8"Team membership changed, reset play");

	curr_play = nullptr;
}

void PlayExecutor::enable_players() {
	const bool enabled[] = {
		enable0,
		enable1,
		enable2,
		enable3,
		enable4,
		enable5,
		enable6,
		enable7,
		enable8,
		enable9,
		enable10,
		enable11,
	};
//	players_enabled.clear();
	players_enabled.insert(players_enabled.begin(), enabled, enabled + 12);
}

