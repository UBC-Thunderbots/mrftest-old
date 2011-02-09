#include "ai/hl/stp/evaluate/evaluate.h"

using AI::HL::STP::Play::Play;

AI::HL::STP::Evaluation::Module::Module(AI::HL::STP::Play::Play& p) : play(p) {
#warning set up signalling to update every tick
}

