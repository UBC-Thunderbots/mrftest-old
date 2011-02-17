#include "ai/hl/stp/play/play.h"
#include "util/dprint.h"

using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayFactory;
using namespace AI::HL::W;

Play::Play(World &world) : world(world) {
}

Play::~Play() {
}

PlayFactory::PlayFactory(const char *name) : Registerable<PlayFactory>(name) {
}

