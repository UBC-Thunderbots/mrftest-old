#include <glibmm/refptr.h>
#include "playerODE.h"	


			playerODE::playerODE (dWorldID eworld) : the_position(0.0, 0.0), the_velocity(0.0, 0.0), target_velocity(0.0, 0.0), the_orientation(0.0), avelocity(0.0) {

				world = eworld;
			}

			void playerODE::update() {

			}

			point playerODE::position() const {
				return the_position;
			}

			double playerODE::orientation() const {
				return the_orientation;
			}

			bool playerODE::has_ball() const {
				return false;
			}

			void playerODE::move_impl(const point &vel, double avel) {
				target_velocity = vel;
				avelocity = avel;
			}

			void playerODE::dribble(double speed) {
			}

			void playerODE::kick(double strength) {
			}

			void playerODE::chip(double strength) {
			}

			void playerODE::ui_set_position(const point &pos) {
			}

		

