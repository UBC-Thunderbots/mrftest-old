#include "geom/point.h"
#include "world/player_impl.h"

namespace {
	class trivial_player_impl : public virtual player_impl {
		public:
			virtual point position() const {
				return point();
			}

			virtual double orientation() const {
				return 0.0;
			}

			virtual bool has_ball() const {
				return false;
			}

			virtual void move_impl(const point &, double) {
			}

			virtual void dribble(double) {
			}

			virtual void kick(double) {
			}

			virtual void chip(double) {
			}

			virtual void ui_set_position(const point &) {
			}
	};
}

const player_impl::ptr &player_impl::trivial() {
	static player_impl::ptr p(new trivial_player_impl);
	return p;
}

