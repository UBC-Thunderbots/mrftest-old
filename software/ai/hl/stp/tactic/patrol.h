#ifndef AI_HL_STP_TACTIC_PATROL_H
#define AI_HL_STP_TACTIC_PATROL_H

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Used for going back and forth between 2 destinations.
			 * A play must store this tactic in order to use this correctly.
			 */
			class Patrol : public Tactic {
				public:
					typedef RefPtr<Patrol> Ptr;

					Patrol(AI::HL::W::World &world);

					void set_waypoints(Point w1, Point w2);

					double score(AI::HL::W::Player::Ptr player);

					void tick(AI::HL::W::Player::Ptr player);

				protected:
					Point p1, p2;
			};
		}
	}
}

#endif

