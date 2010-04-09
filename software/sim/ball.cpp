#include "sim/ball.h"

namespace {
	class trivial_simulator_ball_impl : public simulator_ball_impl {
		public:
			point position() const {
				return point();
			}

			point velocity() const {
				return point();
			}

			point acceleration() const {
				return point();
			}

			void ext_drag(const point &, const point &) {
			}

			bool in_goal() {
				return false;
			}
	};
}

const simulator_ball_impl::ptr &simulator_ball_impl::trivial() {
	static simulator_ball_impl::ptr p(new trivial_simulator_ball_impl);
	return p;
}

