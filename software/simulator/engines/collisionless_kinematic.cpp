#include "geom/angle.h"
#include "simulator/engine.h"
#include "world/timestep.h"

namespace {
	//
	// The limit of floating-point precision.
	//
	const double EPS = 1.0e-9;

	//
	// The maximum acceleration of a robot, in metres per second squared.
	//
	const double BOT_MAX_ACCELERATION = 10.0;

	//
	// The maximum velocity of a robot, in metres per second.
	//
	const double BOT_MAX_VELOCITY = 5.0;

	//
	// The acceleration due to friction against the ball, in metres per second squared.
	//
	const double BALL_DECELERATION = 6.0;

	//
	// A collisionless-kinematic ball_impl.
	//
	class ck_ball : public ball_impl {
		public:
			typedef Glib::RefPtr<ck_ball> ptr;

			ck_ball() : the_position(0.0, 0.0), the_velocity(0.0, 0.0) {
			}

			void kick(const point &direction) {
				the_velocity += direction;
			}

			void tick() {
				the_position += velocity() / static_cast<double>(TIMESTEPS_PER_SECOND);
				const point &velocity_diff = acceleration() / static_cast<double>(TIMESTEPS_PER_SECOND);
				if (velocity_diff.len() >= the_velocity.len())
					the_velocity = point(0.0, 0.0);
				else
					the_velocity += velocity_diff;
			}

			point position() const {
				return the_position;
			}

			void ui_set_position(const point &pos) {
				the_position = pos;
				the_velocity.x = the_velocity.y = 0;
			}

		private:
			point velocity() const {
				return the_velocity;
			}

			point acceleration() const {
				if (the_velocity.len() > EPS) {
					return -the_velocity / the_velocity.len() * BALL_DECELERATION;
				} else {
					return point(0.0, 0.0);
				}
			}

			point the_position, the_velocity;
	};

	//
	// A collisionless-kinematic player_impl.
	//
	class ck_player : public player_impl {
		public:
			typedef Glib::RefPtr<ck_player> ptr;

			ck_player() : the_position(0.0, 0.0), the_velocity(0.0, 0.0), target_velocity(0.0, 0.0), the_orientation(0.0), avelocity(0.0) {
			}

			void tick() {
				the_position += the_velocity;
				const point &diff = target_velocity - the_velocity;
				if (diff.len() < BOT_MAX_ACCELERATION / TIMESTEPS_PER_SECOND) {
					the_velocity = target_velocity;
				} else {
					the_velocity += diff / diff.len() * BOT_MAX_ACCELERATION / static_cast<double>(TIMESTEPS_PER_SECOND);
				}

				the_orientation = angle_mod(the_orientation + avelocity);
			}

			point position() const {
				return the_position;
			}

			double orientation() const {
				return the_orientation;
			}

			bool has_ball() const {
				return false;
			}

			void move_impl(const point &vel, double avel) {
				target_velocity = vel;
double temp = target_velocity.x;
target_velocity.x = -target_velocity.y;
target_velocity.y = temp;
target_velocity = target_velocity/10.0;
				avelocity = avel;
			}

			void dribble(double) {
			}

			void kick(double) {
			}

			void chip(double) {
			}

			void ui_set_position(const point &pos) {
				the_position = pos;
				the_velocity.x = the_velocity.y = 0;
				avelocity = 0;
			}

		private:
			point the_position, the_velocity, target_velocity;
			double the_orientation, avelocity;
	};

	//
	// A collisionless-kinematic simulator_engine.
	//
	class ck_engine : public simulator_engine {
		public:
			ck_engine() : the_ball(new ck_ball) {
			}

			void tick() {
				the_ball->tick();
				for (unsigned int i = 0; i < the_players.size(); i++)
					the_players[i]->tick();
			}

			ball_impl::ptr get_ball() {
				return the_ball;
			}

			player_impl::ptr add_player() {
				ck_player::ptr p(new ck_player);
				the_players.push_back(p);
				return p;
			}

			void remove_player(player_impl::ptr p) {
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (player_impl::ptr::cast_static(the_players[i]) == p) {
						the_players.erase(the_players.begin() + i);
						return;
					}
				}
			}

			Gtk::Widget *get_ui_controls() {
				return 0;
			}

			simulator_engine_factory &get_factory();

		private:
			ck_ball::ptr the_ball;
			std::vector<ck_player::ptr> the_players;
	};

	//
	// A factory for creating ck_engines.
	//
	class ck_engine_factory : public simulator_engine_factory {
		public:
			ck_engine_factory() : simulator_engine_factory("2D Collisionless Kinematic") {
			}

			simulator_engine::ptr create_engine(xmlpp::Element *) {
				simulator_engine::ptr p(new ck_engine);
				return p;
			}
	};

	//
	// The global instance of ck_engine_factory.
	//
	ck_engine_factory fact;

	simulator_engine_factory &ck_engine::get_factory() {
		return fact;
	}
}

