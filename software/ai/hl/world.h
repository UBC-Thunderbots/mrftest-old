#ifndef AI_HL_WORLD_H
#define AI_HL_WORLD_H

#include "ai/flags.h"
#include "ai/backend/backend.h"
#include "ai/common/world.h"
#include "util/property.h"
#include <functional>

namespace AI {
	namespace HL {
		namespace W {
			/**
			 * The field
			 */
			typedef AI::Common::Field Field;

			/**
			 * The ball
			 */
			typedef AI::Common::Ball Ball;

			/**
			 * \brief A robot
			 */
			class Robot : public AI::Common::Robot {
				public:
					/**
					 * \brief This class
					 */
					typedef Robot Ptr;

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
					 * \brief Sets the avoidance distance for this robot
					 *
					 * If this function is not called, the avoidance distance is reset to medium
					 *
					 * This function has no effect on friendly robots
					 *
					 * \param[in] dist the avoidance distance
					 */
					void avoid_distance(AI::Flags::AvoidDistance dist) const;
			};

			/**
			 * \brief A player
			 */
			class Player : public Robot, public AI::Common::Player {
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
					 * \brief Sets the destination for this player
					 *
					 * \param[in] dest the destination position to move to
					 *
					 * \param[in] ori the target orientation to assume
					 *
					 * \param[in] vel the velocity the robot should be moving when it arrives at the target point
					 */
					void move(Point dest, Angle ori, Point vel);

					/**
					 * \brief Returns the movement flags for this player
					 *
					 * \return the flags governing the movement
					 */
					unsigned int flags() const;

					/**
					 * \brief Sets the movement flags for this player
					 *
					 * \param[in] flags the flags governing the movement
					 */
					void flags(unsigned int flags);

					/**
					 * \brief Returns the movement type for this player
					 *
					 * \return the type of movement to perform
					 */
					AI::Flags::MoveType type() const;

					/**
					 * \brief Sets the movement type for this player
					 *
					 * \param[in] type the type of movement to perform
					 */
					void type(AI::Flags::MoveType type);

					/**
					 * \brief Returns the movement priority for this player
					 *
					 * \return the priority of the movement
					 */
					AI::Flags::MovePrio prio() const;

					/**
					 * \brief Sets the movement priority for this player
					 *
					 * \param[in] prio the priority of the movement
					 */
					void prio(AI::Flags::MovePrio prio);

					/**
					 * \brief Causes the player to kick the ball
					 *
					 * \param[in] speed the speed of the kick, in m/s
					 */
					void kick(double speed);

					/**
					 * \brief Causes the player to automatically kick the ball as soon as it is picked up by the sensor
					 *
					 * This function must be called on every tick in order to remain armed; failing to invoke the function will disarm the mechanism
					 *
					 * \param[in] speed the speed of the kick, in m/s
					 */
					void autokick(double speed);

					/**
					 * \brief Causes the player to chip the ball up in the air
					 *
					 * \param[in] power the power of the chip, from 0 to 1 in arbitrary units
					 */
					void chip(double power);

					/**
					 * \brief Causes the player to automatically chip the ball as soon as it is picked up by the sensor
					 *
					 * This function must be called on every tick in order to remain armed; failing to invoke the function will disarm the mechanism
					 *
					 * \param[in] power the power of the chip, from 0 to 1 in arbitrary units
					 */
					void autochip(double power);
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
			 * \brief The world, as seen by a Strategy
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
					 * \brief Returns the current play type
					 *
					 * \return the current play type
					 */
					const Property<AI::Common::PlayType> &playtype() const;

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
	template<> struct less<AI::HL::W::Robot> {
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
			bool operator()(const AI::HL::W::Robot &x, const AI::HL::W::Robot &y) const;

		private:
			std::less<AI::Common::Robot> cmp;
	};

	/**
	 * \brief Provides a total ordering of Player objects so they can be stored in STL ordered containers
	 */
	template<> struct less<AI::HL::W::Player> {
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
			bool operator()(const AI::HL::W::Player &x, const AI::HL::W::Player &y) const;

		private:
			std::less<AI::Common::Player> cmp;
	};
}



inline AI::HL::W::Robot::Robot() = default;

inline AI::HL::W::Robot::Robot(AI::BE::Robot::Ptr impl) : AI::Common::Robot(impl) {
}

inline AI::HL::W::Robot::Robot(const Robot &) = default;

inline AI::HL::W::Robot *AI::HL::W::Robot::operator->() {
	return this;
}

inline const AI::HL::W::Robot *AI::HL::W::Robot::operator->() const {
	return this;
}

inline void AI::HL::W::Robot::avoid_distance(AI::Flags::AvoidDistance dist) const {
	impl->avoid_distance(dist);
}

inline AI::HL::W::Player::Player() = default;

inline AI::HL::W::Player::Player(AI::BE::Player::Ptr impl) : AI::HL::W::Robot(impl), AI::Common::Player(impl) {
}

inline AI::HL::W::Player::Player(const Player &) = default;

inline AI::HL::W::Player *AI::HL::W::Player::operator->() {
	return this;
}

inline const AI::HL::W::Player *AI::HL::W::Player::operator->() const {
	return this;
}

inline void AI::HL::W::Player::move(Point dest, Angle ori, Point vel) {
	AI::Common::Player::impl->move(dest, ori, vel);
}

inline unsigned int AI::HL::W::Player::flags() const {
	return AI::Common::Player::impl->flags();
}

inline void AI::HL::W::Player::flags(unsigned int flags) {
	AI::Common::Player::impl->flags(flags);
}

inline AI::Flags::MoveType AI::HL::W::Player::type() const {
	return AI::Common::Player::impl->type();
}

inline void AI::HL::W::Player::type(AI::Flags::MoveType type) {
	AI::Common::Player::impl->type(type);
}

inline AI::Flags::MovePrio AI::HL::W::Player::prio() const {
	return AI::Common::Player::impl->prio();
}

inline void AI::HL::W::Player::prio(AI::Flags::MovePrio prio) {
	AI::Common::Player::impl->prio(prio);
}

inline void AI::HL::W::Player::kick(double speed) {
	AI::Common::Player::impl->kick(speed);
}

inline void AI::HL::W::Player::autokick(double speed) {
	AI::Common::Player::impl->autokick(speed);
}

inline void AI::HL::W::Player::chip(double power) {
	AI::Common::Player::impl->chip(power);
}

inline void AI::HL::W::Player::autochip(double power) {
	AI::Common::Player::impl->autochip(power);
}

inline AI::HL::W::World::World(AI::BE::Backend &impl) : impl(impl) {
}

inline AI::HL::W::World::World(const World &) = default;

inline const AI::HL::W::Field &AI::HL::W::World::field() const {
	return impl.field();
}

inline const AI::HL::W::Ball &AI::HL::W::World::ball() const {
	return impl.ball();
}

inline AI::HL::W::FriendlyTeam AI::HL::W::World::friendly_team() const {
	return FriendlyTeam(impl.friendly_team());
}

inline AI::HL::W::EnemyTeam AI::HL::W::World::enemy_team() const {
	return EnemyTeam(impl.enemy_team());
}

inline const Property<AI::Common::PlayType> &AI::HL::W::World::playtype() const {
	return impl.playtype();
}

inline bool std::less<AI::HL::W::Robot>::operator()(const AI::HL::W::Robot &x, const AI::HL::W::Robot &y) const {
	return cmp(x, y);
}

inline bool std::less<AI::HL::W::Player>::operator()(const AI::HL::W::Player &x, const AI::HL::W::Player &y) const {
	return cmp(x, y);
}

#endif

