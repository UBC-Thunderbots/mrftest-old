#include "world/player_impl.h"
#include <iostream>

namespace {
	class trivial_player_impl : public player_impl {
		public:
			point position() const {
				return point();
			}

			double orientation() const {
				return 0.0;
			}

			bool has_ball() const {
				return false;
			}

			void move_impl(const point &, double) {
			}

			void dribble(double) {
			}

			void kick(double) {
			}

			void chip(double) {
			}

			void ext_drag(const point &, const point &) {
				ext_drag_postprocess();
			}

			void ext_rotate(double, double) {
				ext_rotate_postprocess();
			}
	};
}

void player_impl::set_controller(robot_controller::ptr c) {
	// Check for a leaky reference to the robot_controller.
	if (controller)
		if (controller->refs() != 1)
			std::cerr << "Leaky reference detected to robot_controller object.\n";

	// Lock in the new controller.
	controller = c;
}

void player_impl::ext_drag_postprocess() {
	destination = position();
}

void player_impl::ext_rotate_postprocess() {
	angular_target = orientation();
}

player_impl::ptr player_impl::trivial() {
	player_impl::ptr p(new trivial_player_impl);
	return p;
}

