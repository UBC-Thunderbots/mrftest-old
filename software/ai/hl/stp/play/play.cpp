#include "ai/hl/stp/play/play.h"
#include "util/dprint.h"

using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayFactory;
using namespace AI::HL::W;

Play::~Play() = default;

bool Play::can_give_up_safely() const {
	return true;
}

void Play::draw_overlay(Cairo::RefPtr<Cairo::Context>) const {
}

Play::Play(World world) : world(world) {
}

PlayFactory::PlayFactory(const char *name) : Registerable<PlayFactory>(name), enable(name, u8"STP/Play/Enable", true), priority(name, u8"STP/Play/Priority 0=low, 10=hi, 5=default", 5, 0, 10), playbook(name, u8"STP/Play/Playbook", 1023, 1, 1023), static_play(name, u8"STP/Play/StaticRoleAssignment", false) {
}

