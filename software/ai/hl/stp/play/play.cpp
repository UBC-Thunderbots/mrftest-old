#include "ai/hl/stp/play/play.h"

using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayManager;
using namespace AI::HL::W;

Play::Play(World &world) : world(world), aborted_(false), timeout_(15.0), change_probability_(1.0) {
	std::time(&start_time);
}

Play::~Play() {
}

double Play::elapsed_time() const {
	std::time_t curr_time;
	std::time(&curr_time);
	return difftime(curr_time, start_time);
}

bool Play::aborted() const {
	return aborted_;
}

void Play::abort() {
	aborted_ = true;
}

double Play::change_probability() const {
	return change_probability_;
}

void Play::set_change_probability(const double p) {
	change_probability_ = p;
}

double Play::timeout() const {
	return timeout_;
}

void Play::set_timeout(const double t) {
	timeout_ = t;
}

PlayManager::PlayManager(const char *name) : Registerable<PlayManager>(name) {
}

PlayManager::~PlayManager() {
}

