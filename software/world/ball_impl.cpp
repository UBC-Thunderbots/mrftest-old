#include "world/ball_impl.h"
#include <glibmm/refptr.h>

namespace {
	class trivial_ball_impl : public virtual ball_impl {
		public:
			virtual point position() const {
				return point();
			}

			virtual point velocity() const {
				return point();
			}

			virtual point acceleration() const {
				return point();
			}

			virtual void ui_set_position(const point &) {
			}
	};
}

const ball_impl::ptr &ball_impl::trivial() {
	static ball_impl::ptr p(new trivial_ball_impl);
	return p;
}

