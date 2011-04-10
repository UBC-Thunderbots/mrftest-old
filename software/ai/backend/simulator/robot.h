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
					Point position() const { return Point(xpred.value(), ypred.value()); }
					Point position(double delta) const { return Point(xpred.value(delta), ypred.value(delta)); }
					Point position(const timespec &ts) const { return Point(xpred.value(ts), ypred.value(ts)); }
					double orientation() const { return tpred.value(); }
					double orientation(double delta) const { return tpred.value(delta); }
					double orientation(const timespec &ts) const { return tpred.value(ts); }
					Point velocity() const { return velocity(0.0); }
					Point velocity(double delta) const { return Point(xpred.value(delta, 1), ypred.value(delta, 1)); }
					Point velocity(const timespec &ts) const { return Point(xpred.value(ts, 1), ypred.value(ts, 1)); }
					double avelocity(double delta) const { return tpred.value(delta, 1); }
					double avelocity(const timespec &ts) const { return tpred.value(ts, 1); }
					Point acceleration(double delta) const { return Point(xpred.value(delta, 2), ypred.value(delta, 2)); }
					Point acceleration(const timespec &ts) const { return Point(xpred.value(ts, 2), ypred.value(ts, 2)); }
					double aacceleration(double delta) const { return tpred.value(delta, 2); }
					double aacceleration(const timespec &ts) const { return tpred.value(ts, 2); }
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
					Robot(unsigned int pattern) : pattern_(pattern), xpred(false), ypred(false), tpred(true) {
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

