#ifndef AI_HL_STP_TACTIC_WAIT_PLAYTYPE_H
#define AI_HL_STP_TACTIC_WAIT_PLAYTYPE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Wait Play Type
				 * Active tactic.
				 * Runs another tactic and waits for a playtype to happen.
				 *
				 * \param[in] tactic the tactic to run.
				 *
				 * \param[in] playtype the playtype to wait for.
				 */
				Tactic::Ptr wait_playtype(World world, Tactic::Ptr &&tactic, const AI::Common::PlayType playtype);
			}
		}
	}
}

#endif

