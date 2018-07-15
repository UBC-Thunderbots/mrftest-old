//
// Created by evan on 19/05/18.
//

#ifndef SOFTWARE_DEFEND_H
#define SOFTWARE_DEFEND_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Tactic
{
/**
 * Goalie defense.
 */
Tactic::Ptr goalie(World world);

Tactic::Ptr goalieAssist1(W::World world);

Tactic::Ptr goalieAssist2(W::World world);

Tactic::Ptr ballerBlocker(W::World world);

}
}
}
}

#endif  // SOFTWARE_DEFEND_H
