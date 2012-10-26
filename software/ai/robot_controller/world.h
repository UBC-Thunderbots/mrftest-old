#ifndef AI_ROBOT_CONTROLLER_WORLD_H
#define AI_ROBOT_CONTROLLER_WORLD_H

#include "ai/flags.h"
#include "ai/backend/backend.h"
#include "ai/common/world.h"
#include "geom/point.h"
#include "util/time.h"
#include <functional>
#include <utility>
#include <vector>

namespace AI {
	namespace RC {
		namespace W {
			/**
			 * \brief A player
			 */
			class Player : public AI::Common::Player, public AI::Common::Robot {
				public:
					/**
					 * \brief The type of a single point in a path
					 */
					typedef std::pair<std::pair<Point, Angle>, timespec> PathPoint;

					/**
					 * \brief The type of a complete path
					 */
					typedef std::vector<PathPoint> Path;

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
					 * \brief Returns the movement flags requested by the Strategy
					 *
					 * \return the flags
					 */
					unsigned int flags() const;

					/**
					 * \brief Returns the movement type requested by the Strategy
					 *
					 * \return the type
					 */
					AI::Flags::MoveType type() const;

					/**
					 * \brief Returns the movement priority requested by the Strategy
					 *
					 * \return the priority
					 */
					AI::Flags::MovePrio prio() const;

					/**
					 * \brief Returns the path requested by the navigator
					 *
					 * \return the path, in the form of a set of
					 * ((<var>position</var>, <var>orientation</var>), <var>deadline</var>) pairs,
					 * where <var>deadline</var> is the timestamp at which the robot should arrive;
					 * the path is empty if the robot is halted
					 */
					const Path &path() const;

					/**
					 * \brief Orders the wheels to turn at specified speeds
					 *
					 * \param[in] w the speeds of the four wheels,
					 * in order front-left, back-left, back-right, front-right,
					 * in units of quarters of a degree of motor shaft rotation per five milliseconds
					 */
					void drive(const int(&w)[4]) const;
			};

			/**
			 * \brief The world
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
					 * \brief Returns the current monotonic time
					 *
					 * Monotonic time is a way of representing “game time”, which always moves forward.
					 * Monotonic time is consistent within the game world, and may or may not be linked to real time.
					 * A navigator should \em always use this function to retrieve monotonic time, not one of the functions in util/time.h!
					 * The AI will not generally have any use for real time.
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
	 * \brief Provides a total ordering of Player objects so they can be stored in STL ordered containers
	 */
	template<> struct less<AI::RC::W::Player> {
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
			bool operator()(const AI::RC::W::Player &x, const AI::RC::W::Player &y) const;

		private:
			std::less<AI::Common::Player> cmp;
	};
}



inline AI::RC::W::Player::Player() = default;

inline AI::RC::W::Player::Player(AI::BE::Player::Ptr impl) : AI::Common::Player(impl), AI::Common::Robot(impl) {
}

inline AI::RC::W::Player::Player(const Player &) = default;

inline AI::RC::W::Player *AI::RC::W::Player::operator->() {
	return this;
}

inline const AI::RC::W::Player *AI::RC::W::Player::operator->() const {
	return this;
}

inline unsigned int AI::RC::W::Player::flags() const {
	return AI::Common::Player::impl->flags();
}

inline AI::Flags::MoveType AI::RC::W::Player::type() const {
	return AI::Common::Player::impl->type();
}

inline AI::Flags::MovePrio AI::RC::W::Player::prio() const {
	return AI::Common::Player::impl->prio();
}

inline const AI::RC::W::Player::Path &AI::RC::W::Player::path() const {
	return AI::Common::Player::impl->path();
}

inline void AI::RC::W::Player::drive(const int(&w)[4]) const {
	AI::Common::Player::impl->drive(w);
}

inline AI::RC::W::World::World(AI::BE::Backend &impl) : impl(impl) {
}

inline AI::RC::W::World::World(const World &) = default;

inline timespec AI::RC::W::World::monotonic_time() const {
	return impl.monotonic_time();
}

inline bool std::less<AI::RC::W::Player>::operator()(const AI::RC::W::Player &x, const AI::RC::W::Player &y) const {
	return cmp(x, y);
}

#endif

