#ifndef AI_BACKEND_SIMULATOR_PLAYER_H
#define AI_BACKEND_SIMULATOR_PLAYER_H

#include "ai/backend/backend.h"
#include "ai/backend/simulator/robot.h"
#include "simulator/sockproto/proto.h"

namespace AI {
	namespace BE {
		namespace Simulator {
			class Backend;

			/**
			 * A friendly robot that exists in a simulator.
			 */
			class Player : public AI::BE::Simulator::Robot, public AI::BE::Player, public sigc::trackable {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;

					/**
					 * A pointer to a const Player.
					 */
					typedef RefPtr<Player> CPtr;

					/**
					 * Constructs a new Player.
					 *
					 * \param[in] be the backend under which the player lives.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 *
					 * \return the new Player.
					 */
					static Ptr create(Backend &be, unsigned int pattern);

					/**
					 * Updates the state of the player and locks in its predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] ts the timestamp at which the robot was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2APlayerInfo &state, const timespec &ts);

					/**
					 * Encodes the robot's current orders into a packet for transmission to the simulator.
					 *
					 * \param[out] orders the packet to encode into.
					 */
					void encode_orders(::Simulator::Proto::A2SPlayerInfo &orders);

					/**
					 * Indicates that the mouse was pressed over the visualizer.
					 *
					 * \param[in] p the point, in world coordinates, over which the mouse was pressed.
					 *
					 * \param[in] btn the number of the button that was pressed.
					 */
					void mouse_pressed(Point p, unsigned int btn);

					/**
					 * Indicates that the mouse was released over the visualizer.
					 *
					 * \param[in] p the point, in world coordinates, over which the mouse was released.
					 *
					 * \param[in] btn the number of the button that was released.
					 */
					void mouse_released(Point p, unsigned int btn);

					/**
					 * Indicates that the mouse exited the area of the visualizer.
					 */
					void mouse_exited();

					/**
					 * Indicates that the mouse was moved over the visualizer.
					 *
					 * \param[in] p the new position of the mouse pointer, in world coordinates.
					 */
					void mouse_moved(Point p);

					Visualizable::Colour visualizer_colour() const;
					Glib::ustring visualizer_label() const { return Robot::visualizer_label(); }
					bool highlight() const;
					Visualizable::Colour highlight_colour() const { return Robot::highlight_colour(); }
					Point position(double delta = 0.0) const { return Robot::position(delta); }
					Point velocity(double delta = 0.0) const { return Robot::velocity(delta); }
					Point acceleration(double delta = 0.0) const { return Robot::acceleration(delta); }
					double orientation(double delta = 0.0) const { return Robot::orientation(delta); }
					double avelocity(double delta = 0.0) const { return Robot::avelocity(delta); }
					double aacceleration(double delta = 0.0) const { return Robot::aacceleration(delta); }
					unsigned int pattern() const { return Robot::pattern(); }
					ObjectStore &object_store() const { return Robot::object_store(); }
					bool has_ball() const;
					bool chicker_ready() const;
					void kick_impl(double power);
					void autokick_impl(double power);
					bool has_destination() const;
					const std::pair<Point, double> &destination() const;
					Point target_velocity() const;
					void path_impl(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p);
					bool has_path() const;
					const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const;
					void drive(const int(&w)[4]);
					const int(&wheel_speeds() const)[4];

				protected:
					/**
					 * Constructs a new Player.
					 *
					 * \param[in] be the backend under which the player lives.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 */
					Player(Backend &be, unsigned int pattern);

					/**
					 * Destroys the Player.
					 */
					~Player();

				private:
					/**
					 * The backend under which the player lives.
					 */
					Backend &be;

					/**
					 * Whether or not this player is holding the ball on its dribbler.
					 */
					bool has_ball_;

					/**
					 * The path of positions, orientations, and deadline timestamps most recently specified by the navigator.
					 */
					std::vector<std::pair<std::pair<Point, double>, timespec> > path_;

					/**
					 * Whether the AI elected to kick in the current time tick.
					 */
					bool kick_;

					/**
					 * The power level for the kick or chip, if one of the flags is set.
					 */
					double chick_power_;

					/**
					 * The rotational speeds requested for the wheels by the robot controller,
					 * in quarters of a degree of motor shaft rotation per five milliseconds.
					 */
					int wheel_speeds_[4];

					/**
					 * The signal connections for mouse activity other than press.
					 */
					sigc::connection mouse_connections[3];

					/**
					 * Disconnects the mouse activity signals other than press.
					 */
					void disconnect_mouse();
			};
		}
	}
}

#endif

