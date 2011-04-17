#include "ai/hl/stp/role.h"

using namespace AI::HL::W;
using AI::HL::STP::Role;

namespace {
	class Fixed : public Role {
		public:
			Fixed(Player::Ptr p) : player(p) {
			}

		private:
			Player::Ptr player;
			Player::Ptr evaluate() const {
				return player;
			}
	};
}

Role::Ptr AI::HL::STP::Role::player(Player::Ptr p) {
	Role::Ptr r(new Fixed(p));
	return r;
}

Role::Role() = default;

