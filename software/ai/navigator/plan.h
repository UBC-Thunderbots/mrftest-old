#include "ai/navigator/world.h"
#include "geom/point.h"

namespace AI{
	namespace Nav{

			class Plan{
			protected:
				AI::Nav::W::World &world;
			public:
				Plan(AI::Nav::W::World &world);
				virtual std::vector<Point> plan(AI::Nav::W::Player::Ptr player, Point goal, unsigned int added_flags=0)=0;
			};
	}
}
