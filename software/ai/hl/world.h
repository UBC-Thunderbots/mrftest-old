#ifndef AI_HL_WORLD_H
#define AI_HL_WORLD_H

#include "drive/robot.h"
#include "ai/flags.h"
#include "ai/backend/backend.h"
#include "ai/common/objects/world.h"
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
					 * \brief Sets the avoidance distance for this robot
					 *
					 * If this function is not called, the avoidance distance is reset to medium
					 *
					 * This function has no effect on friendly robots
					 *
					 * \param[in] dist the avoidance distance
					 */
					void avoid_distance(AI::Flags::AvoidDistance dist);
			};

			/**
			 * \brief A player
			 */
			class Player final : public Robot, public AI::Common::Player {
				public:
					/**
					 * \brief The modes for the dribbler.
					 */
					typedef AI::BE::Player::DribbleMode DribbleMode;

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
					 * \brief Request the player to execute the move primitive and set its destination
					 *
					 * \param[in] dest the destination position to move to
					 */

					void mp_move(Point dest);

					/**
					 * \brief Request the player to execute the move primitive and set its destination
					 *
					 * \param[in] dest the destination position to move to
					 *
					 * \param[in] ori the target orientation to assume
					 */
					void mp_move(Point dest, Angle ori);

					/**
					 * \brief Request the player to execute the dribble primitive and set its destination
					 *
					 * \param[in] dest the destination position to dribble to
					 *
					 * \param[in] ori the target orientation to assume
					 */
					void mp_dribble(Point dest, Angle ori);

					/**
					 * \brief Request the player to execute the move primitive and set its destination
					 *
					 * \param[in] dest the destination position to move to before activating the kicker/chipper
					 *
					 * \param[in] ori the target orientation to assume whent the kick/chipper is activated
					 *
					 * \param[in] chip whether the robot should chip
					 *
					 * \param[in] power kick velocity or chip distance depending on the action
					 */
					void mp_shoot(Point dest, Angle ori, bool chip, double power);

					/**
					 * \brief Request the player to execute the shoot primitive and set its destination. 
					 * This is intended for scenarios where orientation of the kick/chip is not important.
					 *
					 * \param[in] dest the destination position for robot to move to when activating the kicker/chipper
					 *
					 * \param[in] chip whether the robot should chip
					 *
					 * \param[in] power kick velocity or chip distance depending on the action
					 */
					void mp_shoot(Point dest, bool chip, double power);


					/**
					 * \brief Request the player to execute the catch primitive. Interface is not yet set
					 */
					void mp_catch(Point target);// not sure what the best interface is

					/**
					 * \brief Request the player to execute the pivot primitive and set the geometery of the pivot action
					 *
					 * \param[in] centre the point to pivot around
					 *
					 * \param[in] ori final orientation of the robot, assuming it face pivoting centre at the end
					 */
					void mp_pivot(Point centre, Angle ori);

					/**
					 * \brief Request the player to execute the spin primitive and set its destination
					 *
					 * \param[in] dest the destination position to move to
					 *
					 * \param[in] speed angular speed
					 */
					void mp_spin(Point dest, Angle speed);
	

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
			class World final {
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
					 * \brief Compares two world for equality.
					 *
					 * \param[in] w the world to compare to
					 *
					 * \return \c true if this world is equal to \p w, or \c false if not
					 */
					bool operator==(const World &w) const;

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
					Ball ball() const;

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
	template<> struct less<AI::HL::W::Robot> final {
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
	template<> struct less<AI::HL::W::Player> final {
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

inline void AI::HL::W::Robot::avoid_distance(AI::Flags::AvoidDistance dist) {
	impl->avoid_distance(dist);
}

inline AI::HL::W::Player::Player() = default;

inline AI::HL::W::Player::Player(AI::BE::Player::Ptr impl) : AI::HL::W::Robot(impl), AI::Common::Player(impl) {
}

inline AI::HL::W::Player::Player(const Player &) = default;

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

inline AI::HL::W::World::World(AI::BE::Backend &impl) : impl(impl) {
}

inline AI::HL::W::World::World(const World &) = default;

inline const AI::HL::W::Field &AI::HL::W::World::field() const {
	return impl.field();
}

inline AI::HL::W::Ball AI::HL::W::World::ball() const {
	return AI::Common::Ball(impl.ball());
}

inline AI::HL::W::FriendlyTeam AI::HL::W::World::friendly_team() const {
	return FriendlyTeam(impl.friendly_team());
}

inline AI::HL::W::EnemyTeam AI::HL::W::World::enemy_team() const {
	return EnemyTeam(impl.enemy_team());
}

inline bool AI::HL::W::World::operator==(const World &w) const {
	return &impl == &w.impl;
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

inline void AI::HL::W::Player::mp_move(Point dest){
	AI::Common::Player::impl->hl_request.type = Drive::Primitive::MOVE;
	AI::Common::Player::impl->hl_request.field_point = dest;
	//AI::Common::Player::impl->hl_request.field_bool = false; // this means that we don't care about angle
	AI::Common::Player::impl->move(dest, Angle(), Point());
}

inline void AI::HL::W::Player::mp_move(Point dest, Angle ori){
	AI::Common::Player::impl->hl_request.type = Drive::Primitive::MOVE;
	AI::Common::Player::impl->hl_request.field_point = dest;
	AI::Common::Player::impl->hl_request.care_angle = true; // this means that we care about angle
	AI::Common::Player::impl->hl_request.field_angle = ori;
	AI::Common::Player::impl->move(dest, ori, Point());
}

inline void AI::HL::W::Player::mp_dribble(Point dest, Angle ori){
	AI::Common::Player::impl->hl_request.type = Drive::Primitive::DRIBBLE;
	AI::Common::Player::impl->hl_request.field_point = dest;
	AI::Common::Player::impl->hl_request.care_angle = true; // this means that we care about angle
	AI::Common::Player::impl->hl_request.field_angle = ori;
	AI::Common::Player::impl->move(dest, ori, Point());
}

inline void AI::HL::W::Player::mp_shoot(Point dest, Angle ori, bool chip, double power){
	AI::Common::Player::impl->hl_request.type = Drive::Primitive::SHOOT;
	AI::Common::Player::impl->hl_request.field_point = dest;
	AI::Common::Player::impl->hl_request.care_angle = true;
	AI::Common::Player::impl->hl_request.field_angle = ori;
	AI::Common::Player::impl->hl_request.field_bool = chip;
	AI::Common::Player::impl->hl_request.field_double = power;
	AI::Common::Player::impl->move(dest, ori, Point());
}

inline void AI::HL::W::Player::mp_shoot(Point dest, bool chip, double power){
	AI::Common::Player::impl->hl_request.type = Drive::Primitive::SHOOT;
	AI::Common::Player::impl->hl_request.field_point = dest;
	AI::Common::Player::impl->hl_request.care_angle = false; 
	AI::Common::Player::impl->hl_request.field_bool = chip;
	AI::Common::Player::impl->hl_request.field_double = power;
	AI::Common::Player::impl->move(dest, Angle(), Point());
}


inline void AI::HL::W::Player::mp_catch(Point target){
	AI::Common::Player::impl->hl_request.type = Drive::Primitive::CATCH;
	AI::Common::Player::impl->hl_request.field_point = target;
}

inline void AI::HL::W::Player::mp_pivot(Point centre, Angle ori){
	//hl_request.type = 
	AI::Common::Player::impl->hl_request.type = Drive::Primitive::PIVOT;
	AI::Common::Player::impl->hl_request.field_point = centre;
	AI::Common::Player::impl->hl_request.care_angle = true; // this means that we care about angle
	AI::Common::Player::impl->hl_request.field_angle = ori;
	AI::Common::Player::impl->move(centre, ori, Point());
}

inline void AI::HL::W::Player::mp_spin(Point dest, Angle speed){
	//hl_request.type = 
	AI::Common::Player::impl->hl_request.type = Drive::Primitive::SPIN;
	AI::Common::Player::impl->hl_request.field_point = dest;
	AI::Common::Player::impl->hl_request.care_angle = true; // this means that we care about angle
	AI::Common::Player::impl->hl_request.field_angle = speed;
	AI::Common::Player::impl->move(dest, Angle(), Point());
}

#endif

