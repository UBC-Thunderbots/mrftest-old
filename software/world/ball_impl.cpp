#include "world/ball_impl.h"

namespace {
	class trivial_ball_impl : public ball_impl {
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
	};
}

const ball_impl::ptr &ball_impl::trivial() {
	static ball_impl::ptr p(new trivial_ball_impl);
	return p;
}

