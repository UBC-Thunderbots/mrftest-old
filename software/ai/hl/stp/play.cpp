#include "ai/hl/stp/play.h"

using AI::HL::STP::Play;
using AI::HL::STP::PlayManager;
using namespace AI::HL::W;

Play::Play(World& world) : world(world), has_resigned_(false) {
}

Play::~Play() {
}

bool Play::has_resigned() const {
	return has_resigned_;
}

void Play::resign() {
	has_resigned_ = true;
}

PlayManager::PlayManager(const char* name) : Registerable<PlayManager>(name) {
}

