#pragma once

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Offend
				 * Not Active Tactic
				 * Go to good offensive position to park robots.
				 */
				Tactic::Ptr offend(World world);

				/**
				 * Offend Secondary
				 * Not Active Tactic
				 * Go to good offensive position to park robots.
				 */
				Tactic::Ptr offend_secondary(World world);
			}
		}
	}
}
