#ifndef AI_COMMON_ROBOT_H
#define AI_COMMON_ROBOT_H

#include "ai/backend/robot.h"
#include <functional>

namespace AI {
	namespace Common {
		class Robot;
	}
}

namespace std {
	/**
	 * \brief Provides a total ordering of Robot objects so they can be stored in STL ordered containers
	 */
	template<> struct less<AI::Common::Robot> {
		public:
			/**
			 * \brief Compares two objects
			 *
			 * \param[in] x the first objects
			 *
			 * \param[in] y the second objects
			 *
			 * \return \c true if \p x should precede \p y in an ordered container, or \c false if not.
			 */
			bool operator()(const AI::Common::Robot &x, const AI::Common::Robot &y) const;

		private:
			std::less<BoxPtr<AI::BE::Robot>> cmp;
	};
}

namespace AI {
	namespace Common {
		/**
		 * \brief The common functions available on a robot in all layers
		 */
		class Robot {
			public:
				/**
				 * \brief The largest possible radius of a robot, in metres
				 */
				static const double MAX_RADIUS;

				/**
				 * \brief Constructs a nonexistent Robot
				 */
				Robot();

				/**
				 * \brief Constructs a new Robot
				 *
				 * \param[in] impl the backend implementation
				 */
				Robot(AI::BE::Robot::Ptr impl);

				/**
				 * \brief Copies a Robot
				 *
				 * \param[in] copyref the object to copy
				 */
				Robot(const Robot &copyref);

				/**
				 * \brief Checks whether two robots are equal
				 *
				 * \param[in] other the robot to compare to
				 *
				 * \return \c true if the objects refer to the same robot, or \c false if not
				 */
				bool operator==(const Robot &other) const;

				/**
				 * \brief Checks whether two robots are equal
				 *
				 * \param[in] other the robot to compare to
				 *
				 * \return \c false if the objects refer to the same robot, or \c true if not
				 */
				bool operator!=(const Robot &other) const;

				/**
				 * \brief Checks whether the robot exists
				 *
				 * \return \c true if the object refers to an existing robot, or \c false if not
				 */
				explicit operator bool() const;

				/**
				 * \brief Returns the index of the robot
				 *
				 * \return the index of the robot's lid pattern
				 */
				unsigned int pattern() const;

				/**
				 * \brief Returns an object store for the robot
				 *
				 * AI entities can use this object store to hold opaque data on a per-robot basis,
				 * without worrying about keeping parallel data structures up-to-date and dealing with team changes.
				 *
				 * \return an object store
				 */
				ObjectStore &object_store() const;

				/**
				 * \brief Gets the predicted position of the object
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the predicted position
				 */
				Point position(double delta = 0.0) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the standard deviation of the predicted position of the object
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the standard deviation of the predicted position
				 */
				Point position_stdev(double delta = 0.0) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the predicted velocity of the object
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the predicted velocity
				 */
				Point velocity(double delta = 0.0) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the standard deviation of the predicted velocity of the object
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the standard deviation of the predicted velocity
				 */
				Point velocity_stdev(double delta = 0.0) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the predicted orientation of the object
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the predicted orientation
				 */
				Angle orientation(double delta = 0.0) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the standard deviation of the predicted orientation of the object
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the standard deviation of the predicted orientation
				 */
				Angle orientation_stdev(double delta = 0.0) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the predicted angular velocity of the object
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the predicted angular velocity
				 */
				Angle avelocity(double delta = 0.0) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the standard deviation of the predicted angular velocity of the object
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the standard deviation of the predicted angular velocity
				 */
				Angle avelocity_stdev(double delta = 0.0) const __attribute__((warn_unused_result));

			protected:
				AI::BE::Robot::Ptr impl;

				friend struct std::less<Robot>;
		};
	}
}



inline bool std::less<AI::Common::Robot>::operator()(const AI::Common::Robot &x, const AI::Common::Robot &y) const {
	return cmp(x.impl, y.impl);
}

inline AI::Common::Robot::Robot() = default;

inline AI::Common::Robot::Robot(AI::BE::Robot::Ptr impl) : impl(impl) {
}

inline AI::Common::Robot::Robot(const Robot &) = default;

inline bool AI::Common::Robot::operator==(const Robot &other) const {
	return impl == other.impl;
}

inline bool AI::Common::Robot::operator!=(const Robot &other) const {
	return !(*this == other);
}

inline AI::Common::Robot::operator bool() const {
	return !!impl;
}

inline unsigned int AI::Common::Robot::pattern() const {
	return impl->pattern();
}

inline ObjectStore &AI::Common::Robot::object_store() const {
	return impl->object_store();
}

inline Point AI::Common::Robot::position(double delta) const {
	return impl->position(delta);
}

inline Point AI::Common::Robot::position_stdev(double delta) const {
	return impl->position_stdev(delta);
}

inline Point AI::Common::Robot::velocity(double delta) const {
	return impl->velocity(delta);
}

inline Point AI::Common::Robot::velocity_stdev(double delta) const {
	return impl->velocity_stdev(delta);
}

inline Angle AI::Common::Robot::orientation(double delta) const {
	return impl->orientation(delta);
}

inline Angle AI::Common::Robot::orientation_stdev(double delta) const {
	return impl->orientation_stdev(delta);
}

inline Angle AI::Common::Robot::avelocity(double delta) const {
	return impl->avelocity(delta);
}

inline Angle AI::Common::Robot::avelocity_stdev(double delta) const {
	return impl->avelocity_stdev(delta);
}

#endif

