#ifndef AI_HL_STP_TACTIC_WAIT_PLAYTYPE_H
#define AI_HL_STP_TACTIC_WAIT_PLAYTYPE_H

#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Runs another tactic and waits for a playtype to happen.
				 * Active tactic.
				 *
				 * \param[in] tactic the tactic to run.
				 *
				 * \param[in] playtype the playtype to wait for.
				 */
				Tactic::Ptr wait_playtype(const World &world, Tactic::Ptr tactic, const PlayType::PlayType playtype);
			}
		}
	}
}

#endif

