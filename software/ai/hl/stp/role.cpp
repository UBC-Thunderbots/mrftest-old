#include "ai/hl/stp/role.h"

using namespace AI::HL::W;
using AI::HL::STP::Role;

namespace
{
class Fixed final : public Role
{
   public:
    explicit Fixed(Player p) : player(p)
    {
    }

   private:
    Player player;
    Player evaluate() const override
    {
        return player;
    }
};
}

Role::Ptr AI::HL::STP::Role::player(Player p)
{
    return std::make_shared<Fixed>(p);
}

Role::Role() = default;
