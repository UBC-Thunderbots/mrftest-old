#ifndef AI_BACKEND_SIMULATOR_ROBOT_H
#define AI_BACKEND_SIMULATOR_ROBOT_H

#include "ai/backend/backend.h"
#include "simulator/sockproto/proto.h"
#include "util/byref.h"
#include "util/predictor.h"
#include <stdexcept>

namespace AI {
	namespace BE {
		namespace Simulator {
			/**
			 * A robot that exists within the simulator, which may be friendly or enemy.
			 */
			class Robot : public AI::BE::Robot {
				public:
					/**
					 * A pointer to a Robot.
					 */
					typedef RefPtr<Robot> Ptr;

					/**
					 * A pointer to a const Robot.
					 */
					typedef RefPtr<const Robot> CPtr;

					/**
					 * Constructs a new Robot.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 *
					 * \return the new Robot.
					 */
					static Ptr create(unsigned int pattern) {
						Ptr p(new Robot(pattern));
						return p;
					}

					/**
					 * Updates the state of the player and locks in its predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] ts the timestamp at which the robot was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2ARobotInfo &state, const timespec &ts) {
						AI::BE::Robot::pre_tick();
						pred.add_measurement(Point(state.x, state.y), Angle::of_radians(state.orientation), ts);
						pred.lock_time(ts);
					}

					ObjectStore &object_store() const { return object_store_; }
					unsigned int pattern() const { return pattern_; }
					Point position(double delta = 0.0) const { return pred.value(delta).first.first; }
					Angle orientation(double delta = 0.0) const { return pred.value(delta).first.second; }
					Point velocity(double delta = 0.0) const { return pred.value(delta, 1).first.first; }
					Angle avelocity(double delta = 0.0) const { return pred.value(delta, 1).first.second; }
					Point position_stdev(double delta = 0.0) const { return pred.value(delta).second.first; }
					Angle orientation_stdev(double delta = 0.0) const { return pred.value(delta).second.second; }
					Point velocity_stdev(double delta = 0.0) const { return pred.value(delta, 1).second.first; }
					Angle avelocity_stdev(double delta = 0.0) const { return pred.value(delta, 1).second.second; }
					Visualizable::Colour visualizer_colour() const { return Visualizable::Colour(1.0, 0.0, 0.0); }
					Glib::ustring visualizer_label() const { return Glib::ustring::format(pattern_); }
					bool highlight() const { return false; }
					Visualizable::Colour highlight_colour() const { return Visualizable::Colour(0.0, 0.0, 0.0); }
					bool has_destination() const { return false; }
					const std::pair<Point, Angle> &destination() const { throw std::logic_error("This robot has no destination"); }
					bool has_path() const { return false; }
					const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &path() const { throw std::logic_error("This robot has no path"); }
					unsigned int num_bar_graphs() const { return 0; }
					double bar_graph_value(unsigned int) const { return 0.0; }
					Visualizable::Colour bar_graph_colour(unsigned int) const { return Visualizable::Colour(0.0, 0.0, 0.0); }

				protected:
					/**
					 * Constructs a new Robot.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 */
					Robot(unsigned int pattern);

					/**
					 * Destroys a Robot.
					 */
					~Robot() {
					}

				private:
					/**
					 * The lid pattern.
					 */
					const unsigned int pattern_;

					/**
					 * Predicts robot movement.
					 */
					Predictor3 pred;

					/**
					 * The object store that holds private data for the rest of the stack that is specific to this robot.
					 */
					mutable ObjectStore object_store_;
			};
		}
	}
}

#endif

