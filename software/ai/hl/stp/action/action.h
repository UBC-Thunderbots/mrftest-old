#pragma once

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace Primitives = AI::BE::Primitives;
using caller_t = AI::HL::STP::Tactic::Tactic::caller_t;
using Primitives::Primitive;

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				inline void yield(caller_t& ca) {
					ca();
				}

				inline void wait(caller_t& ca, const Primitive& prim) {
					while (!prim.done()) {
						yield(ca);
					}
				}
			}
		}
	}
}
