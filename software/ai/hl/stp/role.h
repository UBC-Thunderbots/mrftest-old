#ifndef AI_HL_STP_ROLE_H
#define AI_HL_STP_ROLE_H

#include <utility>
#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/world.h"
#include "util/noncopyable.h"

namespace AI
{
namespace HL
{
namespace STP
{
/**
 * \brief This class allows various ways to refer to own players.
 *
 * By player, tactic, or role.
 */
class Role : public NonCopyable
{
   public:
    /**
     * \brief A pointer to a Role.
     */
    typedef std::shared_ptr<Role> Ptr;

    /**
     * \brief Gets the player associated with this role.
     */
    virtual Player evaluate() const = 0;

    /**
     * \brief A specific player.
     */
    static Role::Ptr player(Player player);

    /**
     * \brief A specific tactic.
     */
    static Role::Ptr tactic(const AI::HL::STP::Tactic::Tactic::Ptr &tactic);

    /**
     * \brief Ordered by priority.
     */
    static Role::Ptr role(unsigned int i);

    /**
     * \brief The goalie.
     */
    static Role::Ptr goalie(World world);

   protected:
    explicit Role();
};
}
}
}

#endif
