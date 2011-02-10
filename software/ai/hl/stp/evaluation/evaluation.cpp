#include "ai/hl/stp/evaluation/evaluation.h"

using AI::HL::STP::Play::Play;
using AI::HL::STP::Evaluation::Module;

Module::Module(AI::HL::STP::Play::Play& p) : play(p) {
	play.register_module(*this);
}

Module::~Module() {
	connection.disconnect();
}

