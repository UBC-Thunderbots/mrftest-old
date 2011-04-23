#include "ai/hl/stp/play/play.h"
#include "util/dprint.h"

using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayFactory;
using namespace AI::HL::W;

Play::Play(const World &world) : world(world) {
}

void Play::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
}

PlayFactory::PlayFactory(const char *name) : Registerable<PlayFactory>(name) {
}

