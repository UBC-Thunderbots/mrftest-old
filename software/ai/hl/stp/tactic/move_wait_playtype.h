#ifndef AI_HL_STP_TACTIC_MOVE_WAIT_PLAYTYPE_H
#define AI_HL_STP_TACTIC_MOVE_WAIT_PLAYTYPE_H

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Active tactic.
				 * Move to position, and waits for a particular playtype.
				 *
				 * \param[in] playtype the playtype to wait for.
				 */
				Tactic::Ptr move_wait_playtype(const World &world, const Coordinate dest, const PlayType::PlayType playtype);
			}
		}
	}
}

#endif

