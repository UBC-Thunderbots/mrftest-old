#include "ai/hl/stp/region.h"

Point AI::HL::STP::Rectangle::center() const {
	return (p1() + p2()) / 2;
}

