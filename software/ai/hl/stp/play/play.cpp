#include "ai/hl/stp/play/play.h"

using AI::HL::STP::Play;
using AI::HL::STP::PlayManager;
using namespace AI::HL::W;

Play::Play(World& world, const char* name) : world(world), name_(name), has_resigned_(false) {
}

Play::~Play() {
}

bool Play::has_resigned() const {
	return has_resigned_;
}

const std::string& Play::name() const {
	return name_;
}

void Play::resign() {
	has_resigned_ = true;
}

PlayManager::PlayManager(const char* name) : Registerable<PlayManager>(name) {
}

PlayManager::~PlayManager() {
}

