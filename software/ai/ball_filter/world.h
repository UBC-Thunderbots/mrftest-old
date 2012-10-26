#ifndef AI_BALL_FILTER_WORLD_H
#define AI_BALL_FILTER_WORLD_H

#include "ai/backend/backend.h"
#include "ai/common/world.h"
#include <functional>

namespace AI {
	namespace BF {
		namespace W {
			/**
			 * \brief The field
			 */
			typedef AI::Common::Field Field;

			/**
			 * \brief The ball
			 */
			typedef AI::Common::Ball Ball;

			/**
			 * \brief A robot
			 */
			typedef AI::Common::Robot Robot;

			/**
			 * \brief A player
			 */
			class Player : public Robot, public AI::Common::Player {
				public:
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
			};

			/**
			 * \brief The friendly team
			 */
			typedef AI::Common::Team<Player, AI::BE::Player> FriendlyTeam;

			/**
			 * \brief The enemy team.
			 */
			typedef AI::Common::Team<Robot, AI::BE::Robot> EnemyTeam;

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
	template<> struct less<AI::BF::W::Player> {
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
			bool operator()(const AI::BF::W::Player &x, const AI::BF::W::Player &y) const;

		private:
			std::less<AI::Common::Player> cmp;
	};
}



inline AI::BF::W::World::World(AI::BE::Backend &impl) : impl(impl) {
}

inline AI::BF::W::World::World(const World &) = default;

inline const AI::BF::W::Field &AI::BF::W::World::field() const {
	return impl.field();
}

inline const AI::BF::W::Ball &AI::BF::W::World::ball() const {
	return impl.ball();
}

inline AI::BF::W::FriendlyTeam AI::BF::W::World::friendly_team() const {
	return FriendlyTeam(impl.friendly_team());
}

inline AI::BF::W::EnemyTeam AI::BF::W::World::enemy_team() const {
	return EnemyTeam(impl.enemy_team());
}

inline bool std::less<AI::BF::W::Player>::operator()(const AI::BF::W::Player &x, const AI::BF::W::Player &y) const {
	return cmp(x, y);
}

#endif

