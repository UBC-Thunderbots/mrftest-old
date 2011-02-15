#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/evaluation/evaluation.h"
#include "util/dprint.h"

using AI::HL::STP::Play::Play;
using AI::HL::STP::Play::PlayFactory;
using namespace AI::HL::W;
using AI::HL::STP::Evaluation::Module;

Play::Play(World &world) : world(world) {
}

Play::~Play() {
}

void Play::register_module(Module &module) {
	if (&module.play != this) {
		LOG_ERROR("Can't register a module not belonging to the right play");
		return;
	}
	module.connection = signal_tick.connect(sigc::mem_fun(module, &Module::evaluate));
}

void Play::tick() {
	signal_tick.emit();
}

PlayFactory::PlayFactory(const char *name) : Registerable<PlayFactory>(name) {
}

