#ifndef AI_NAVIGATOR_WORLD_H
#define AI_NAVIGATOR_WORLD_H

#include "ai/backend/backend.h"
#include "ai/common/world.h"
#include "ai/flags.h"
#include <functional>
#include <utility>
#include <vector>

namespace AI {
	namespace Nav {
		namespace W {
			/**
			 * \brief The field, as seen by a Navigator
			 */
			typedef AI::Common::Field Field;

			/**
			 * \brief The ball, as seen by a Navigator
			 */
			typedef AI::Common::Ball Ball;

			/**
			 * \brief A robot, as seen by a Navigator
			 */
			class Robot : public AI::Common::Robot {
				public:
					/**
					 * \brief This class
					 */
					typedef Robot Ptr;

					/**
					 * \brief The largest possible radius of a robot, in metres
					 */
					static const double MAX_RADIUS;

					/**
					 * \brief Constructs a nonexistent Robot
					 */
					explicit Robot();

					/**
					 * \brief Constructs a new Robot
					 *
					 * \param[in] impl the backend implementation to wrap
					 */
					explicit Robot(AI::BE::Robot::Ptr impl);

					/**
					 * \brief Constructs a copy of a Robot
					 *
					 * \param[in] copyref the object to copy
					 */
					Robot(const Robot &copyref);

					/**
					 * \brief Returns this object
					 *
					 * \return this object
					 */
					Robot *operator->();

					/**
					 * \brief Returns this object
					 *
					 * \return this object
					 */
					const Robot *operator->() const;

					/**
					 * \brief Returns the avoidance distance for this robot
					 *
					 * \return the avoidance distance
					 */
					AI::Flags::AvoidDistance avoid_distance() const;
			};

			/**
			 * \brief A player, as seen by a Navigator
			 */
			class Player : public AI::Common::Player, public Robot {
				public:
					/**
					 * \brief This class
					 */
					typedef Player Ptr;

					/**
					 * \brief This class, const
					 */
					typedef Player CPtr;

					/**
					 * \brief The type of a single point in a path
					 */
					typedef std::pair<std::pair<Point, Angle>, timespec> PathPoint;

					/**
					 * \brief The type of a complete path
					 */
					typedef std::vector<PathPoint> Path;

					/**
					 * \brief The maximum linear velocity of the robot, in metres per second
					 */
					static const double MAX_LINEAR_VELOCITY;

					/**
					 * \brief The maximum linear acceleration of the robot, in metres per second squared
					 */
					static const double MAX_LINEAR_ACCELERATION;

					/**
					 * \brief The maximum angular velocity of the robot per second
					 */
					static const Angle MAX_ANGULAR_VELOCITY;

					/**
					 * \brief The maximum angular acceleration of the robot per second squared
					 */
					static const Angle MAX_ANGULAR_ACCELERATION;

					/**
					 * \brief Constructs a nonexistent Player
					 */
					explicit Player();

					/**
					 * \brief Constructs a new Player
					 *
					 * \param[in] impl the backend implementation to wrap
					 */
					explicit Player(AI::BE::Player::Ptr impl);

					/**
					 * \brief Constructs a copy of a Player
					 *
					 * \param[in] copyref the object to copy
					 */
					Player(const Player &copyref);

					/**
					 * \brief Returns this object
					 *
					 * \return this object
					 */
					Player *operator->();

					/**
					 * \brief Returns this object
					 *
					 * \return this object
					 */
					const Player *operator->() const;

					using AI::Common::Player::operator==;
					using AI::Common::Player::operator!=;
					using AI::Common::Player::operator bool;

					/**
					 * \brief Returns the destination position and orientation requested by the HighLevel
					 *
					 * \return the destination position and orientation
					 */
					std::pair<Point, Angle> destination() const;

					/**
					 * \brief Returns the target velocity requested by the HighLevel
					 *
					 * \return the target velocity
					 */
					Point target_velocity() const;

					/**
					 * \brief Returns the movement flags requested by the HighLevel
					 *
					 * \return the flags
					 */
					unsigned int flags() const;

					/**
					 * \brief Returns the movement type requested by the HighLevel
					 *
					 * \return the type
					 */
					AI::Flags::MoveType type() const;

					/**
					 * \brief Returns the movement priority requested by the HighLevel
					 *
					 * \return the priority
					 */
					AI::Flags::MovePrio prio() const;

					/**
					 * \brief Sets the path this player should follow
					 *
					 * \param[in] p the path (an empty path causes the robot to halt)
					 */
					void path(const Path &p);
			};

			/**
			 * \brief The friendly team
			 */
			typedef AI::Common::Team<Player, AI::BE::Player> FriendlyTeam;

			/**
			 * \brief The enemy team
			 */
			typedef AI::Common::Team<Robot, AI::BE::Robot> EnemyTeam;

			/**
			 * \brief The world, as seen by a Navigator
			 */
			class World {
				public:
					/**
					 * \brief Constructs a new World
					 *
					 * \param[in] impl the backend implementation
					 */
					explicit World(AI::BE::Backend &impl);

					/**
					 * \brief Constructs a copy of a World
					 *
					 * \param[in] copyref the object to copy
					 */
					World(const World &copyref);

					/**
					 * \brief Returns the field
					 *
					 * \return the field
					 */
					const Field &field() const;

					/**
					 * \brief Returns the ball
					 *
					 * \return the ball
					 */
					const Ball &ball() const;

					/**
					 * \brief Returns the friendly team
					 *
					 * \return the friendly team
					 */
					FriendlyTeam friendly_team() const;

					/**
					 * \brief Returns the enemy team
					 *
					 * \return the enemy team
					 */
					EnemyTeam enemy_team() const;

					/**
					 * \brief Returns the current monotonic time
					 *
					 * Monotonic time is a way of representing "game time", which always moves forward
					 * Monotonic time is consistent within the game world, and may or may not be linked to real time
					 * A navigator should \em always use this function to retrieve monotonic time, not one of the functions in util/time.h!
					 * The AI will not generally have any use for real time
					 *
					 * \return the current monotonic time
					 */
					timespec monotonic_time() const;

				private:
					AI::BE::Backend &impl;
			};
		}
	}
}

namespace std {
	/**
	 * \brief Provides a total ordering of Robot objects so they can be stored in STL ordered containers
	 */
	template<> struct less<AI::Nav::W::Robot> {
		public:
			/**
			 * \brief Compares two objects
			 *
			 * \param[in] x the first objects
			 *
			 * \param[in] y the second objects
			 *
			 * \return \c true if \p x should precede \p y in an ordered container, or \c false if not
			 */
			bool operator()(const AI::Nav::W::Robot &x, const AI::Nav::W::Robot &y) const;

		private:
			std::less<AI::Common::Robot> cmp;
	};

	/**
	 * \brief Provides a total ordering of Player objects so they can be stored in STL ordered containers
	 */
	template<> struct less<AI::Nav::W::Player> {
		public:
			/**
			 * \brief Compares two objects
			 *
			 * \param[in] x the first objects
			 *
			 * \param[in] y the second objects
			 *
			 * \return \c true if \p x should precede \p y in an ordered container, or \c false if not
			 */
			bool operator()(const AI::Nav::W::Player &x, const AI::Nav::W::Player &y) const;

		private:
			std::less<AI::Common::Player> cmp;
	};
}



inline AI::Nav::W::Robot::Robot() = default;

inline AI::Nav::W::Robot::Robot(AI::BE::Robot::Ptr impl) : AI::Common::Robot(impl) {
}

inline AI::Nav::W::Robot::Robot(const Robot &) = default;

inline AI::Nav::W::Robot *AI::Nav::W::Robot::operator->() {
	return this;
}

inline const AI::Nav::W::Robot *AI::Nav::W::Robot::operator->() const {
	return this;
}

inline AI::Flags::AvoidDistance AI::Nav::W::Robot::avoid_distance() const {
	return impl->avoid_distance();
}

inline AI::Nav::W::Player::Player() = default;

inline AI::Nav::W::Player::Player(AI::BE::Player::Ptr impl) : AI::Common::Player(impl), AI::Nav::W::Robot(impl) {
}

inline AI::Nav::W::Player::Player(const Player &) = default;

inline AI::Nav::W::Player *AI::Nav::W::Player::operator->() {
	return this;
}

inline const AI::Nav::W::Player *AI::Nav::W::Player::operator->() const {
	return this;
}

inline std::pair<Point, Angle> AI::Nav::W::Player::destination() const {
	return AI::Common::Player::impl->destination();
}

inline Point AI::Nav::W::Player::target_velocity() const {
	return AI::Common::Player::impl->target_velocity();
}

inline unsigned int AI::Nav::W::Player::flags() const {
	return AI::Common::Player::impl->flags();
}

inline AI::Flags::MoveType AI::Nav::W::Player::type() const {
	return AI::Common::Player::impl->type();
}

inline AI::Flags::MovePrio AI::Nav::W::Player::prio() const {
	return AI::Common::Player::impl->prio();
}

inline void AI::Nav::W::Player::path(const Path &p) {
	AI::Common::Player::impl->path(p);
}

inline AI::Nav::W::World::World(AI::BE::Backend &impl) : impl(impl) {
}

inline AI::Nav::W::World::World(const World &) = default;

inline const AI::Nav::W::Field &AI::Nav::W::World::field() const {
	return impl.field();
}

inline const AI::Nav::W::Ball &AI::Nav::W::World::ball() const {
	return impl.ball();
}

inline AI::Nav::W::FriendlyTeam AI::Nav::W::World::friendly_team() const {
	return FriendlyTeam(impl.friendly_team());
}

inline AI::Nav::W::EnemyTeam AI::Nav::W::World::enemy_team() const {
	return EnemyTeam(impl.enemy_team());
}

inline timespec AI::Nav::W::World::monotonic_time() const {
	return impl.monotonic_time();
}

inline bool std::less<AI::Nav::W::Robot>::operator()(const AI::Nav::W::Robot &x, const AI::Nav::W::Robot &y) const {
	return cmp(x, y);
}

inline bool std::less<AI::Nav::W::Player>::operator()(const AI::Nav::W::Player &x, const AI::Nav::W::Player &y) const {
	return cmp(x, y);
}

#endif

