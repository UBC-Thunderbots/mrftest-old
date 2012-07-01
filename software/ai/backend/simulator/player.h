#ifndef AI_BACKEND_SIMULATOR_PLAYER_H
#define AI_BACKEND_SIMULATOR_PLAYER_H

#include "ai/backend/backend.h"
#include "ai/backend/simulator/robot.h"
#include "simulator/sockproto/proto.h"
#include "util/box_ptr.h"

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
					typedef BoxPtr<Player> Ptr;

					/**
					 * A pointer to a const Player.
					 */
					typedef BoxPtr<Player> CPtr;

					/**
					 * Constructs a new Player.
					 *
					 * \param[in] be the backend under which the player lives.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 */
					explicit Player(Backend &be, unsigned int pattern);

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
					Glib::ustring visualizer_label() const { return AI::BE::Simulator::Robot::visualizer_label(); }
					bool highlight() const;
					Visualizable::Colour highlight_colour() const { return AI::BE::Simulator::Robot::highlight_colour(); }
					Point position(double delta = 0.0) const { return AI::BE::Simulator::Robot::position(delta); }
					Point velocity(double delta = 0.0) const { return AI::BE::Simulator::Robot::velocity(delta); }
					Angle orientation(double delta = 0.0) const { return AI::BE::Simulator::Robot::orientation(delta); }
					Angle avelocity(double delta = 0.0) const { return AI::BE::Simulator::Robot::avelocity(delta); }
					Point position_stdev(double delta = 0.0) const { return AI::BE::Simulator::Robot::position_stdev(delta); }
					Point velocity_stdev(double delta = 0.0) const { return AI::BE::Simulator::Robot::velocity_stdev(delta); }
					Angle orientation_stdev(double delta = 0.0) const { return AI::BE::Simulator::Robot::orientation_stdev(delta); }
					Angle avelocity_stdev(double delta = 0.0) const { return AI::BE::Simulator::Robot::avelocity_stdev(delta); }
					unsigned int pattern() const { return AI::BE::Simulator::Robot::pattern(); }
					ObjectStore &object_store() const { return AI::BE::Simulator::Robot::object_store(); }
					bool has_chipper() const;
					bool alive() const;
					bool has_ball() const;
					bool chicker_ready() const;
					void kick_impl(double speed);
					void autokick_impl(double speed);
					void chip_impl(double) { *(reinterpret_cast<volatile int *>(0)) = 27; }
					void autochip_impl(double) { *(reinterpret_cast<volatile int *>(0)) = 27; }
					bool autokick_fired() const;
					bool has_destination() const;
					const std::pair<Point, Angle> &destination() const;
					Point target_velocity() const;
					void path_impl(const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &p);
					bool has_path() const;
					const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &path() const;
					unsigned int num_bar_graphs() const { return AI::BE::Simulator::Robot::num_bar_graphs(); }
					double bar_graph_value(unsigned int index) const { return AI::BE::Simulator::Robot::bar_graph_value(index); }
					Visualizable::Colour bar_graph_colour(unsigned int index) const { return AI::BE::Simulator::Robot::bar_graph_colour(index); }
					void drive(const int(&w)[4]);
					const int(&wheel_speeds() const)[4];
					void avoid_distance(AI::Flags::AvoidDistance dist) const { AI::BE::Simulator::Robot::avoid_distance(dist); }
					AI::Flags::AvoidDistance avoid_distance() const { return AI::BE::Simulator::Robot::avoid_distance(); }
					using AI::BE::Player::pre_tick;
					using AI::BE::Player::path;

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
					std::vector<std::pair<std::pair<Point, Angle>, timespec> > path_;

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
					 * \brief Whether or not the AI elected to autokick in the last time tick.
					 */
					bool autokick_fired_;

					/**
					 * \brief Whether or not the AI elected to autokick in the current time tick.
					 */
					bool autokick_pre_fired_;

					/**
					 * Disconnects the mouse activity signals other than press.
					 */
					void disconnect_mouse();
			};
		}
	}
}

#endif

