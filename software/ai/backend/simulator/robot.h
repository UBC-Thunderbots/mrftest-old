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
						xpred.add_datum(state.x, ts);
						xpred.lock_time(ts);
						ypred.add_datum(state.y, ts);
						ypred.lock_time(ts);
						tpred.add_datum(state.orientation, ts);
						tpred.lock_time(ts);
					}

					ObjectStore &object_store() const { return object_store_; }
					unsigned int pattern() const { return pattern_; }
					Point position(double delta = 0.0) const { return Point(xpred.value(delta).first, ypred.value(delta).first); }
					double orientation(double delta = 0.0) const { return tpred.value(delta).first; }
					Point velocity(double delta = 0.0) const { return Point(xpred.value(delta, 1).first, ypred.value(delta, 1).first); }
					double avelocity(double delta = 0.0) const { return tpred.value(delta, 1).first; }
					Visualizable::Colour visualizer_colour() const { return Visualizable::Colour(1.0, 0.0, 0.0); }
					Glib::ustring visualizer_label() const { return Glib::ustring::format(pattern_); }
					bool highlight() const { return false; }
					Visualizable::Colour highlight_colour() const { return Visualizable::Colour(0.0, 0.0, 0.0); }
					bool has_destination() const { return false; }
					const std::pair<Point, double> &destination() const { throw std::logic_error("This robot has no destination"); }
					bool has_path() const { return false; }
					const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const { throw std::logic_error("This robot has no path"); }

				protected:
					/**
					 * Constructs a new Robot.
					 *
					 * \param[in] pattern the pattern index of the robot.
					 */
					Robot(unsigned int pattern) : pattern_(pattern), xpred(false, 1.3e-3, 2), ypred(false, 1.3e-3, 2), tpred(true, 1.3e-3, 2) {
					}

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
					 * A predictor that provides the X coordinate of predictable quantities.
					 */
					Predictor xpred;

					/**
					 * A predictor that provides the Y coordinate of predictable quantities.
					 */
					Predictor ypred;

					/**
					 * A predictor that provides predictable quantities around orientation.
					 */
					Predictor tpred;

					/**
					 * The object store that holds private data for the rest of the stack that is specific to this robot.
					 */
					mutable ObjectStore object_store_;
			};
		}
	}
}

#endif

