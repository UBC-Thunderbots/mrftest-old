#ifndef AI_BACKEND_BACKEND_H
#define AI_BACKEND_BACKEND_H

#include "ai/ball_filter/ball_filter.h"
#include "ai/hl/hl.h"
#include "ai/hl/world.h"
#include "ai/navigator/world.h"
#include "ai/robot_controller/world.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "uicomponents/visualizer.h"
#include "util/config.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include "util/registerable.h"
#include <glibmm.h>
#include <map>
#include <sigc++/sigc++.h>

namespace Gtk {
	class Widget;
}

namespace AI {
	namespace BE {
		/**
		 * The field, as exposed by the backend.
		 */
		class Field : public AI::BF::W::Field, public AI::HL::W::Field, public AI::Nav::W::Field, public Visualizable::Field {
			public:
				/**
				 * Checks if the field data is valid yet.
				 *
				 * \return \c true if the data in the Field is valid, or \c false if not.
				 */
				virtual bool valid() const = 0;
		};

		/**
		 * The ball, as exposed by the backend.
		 */
		class Ball : public AI::BF::W::Ball, public AI::HL::W::Ball, public AI::Nav::W::Ball, public Visualizable::Ball {
			public:
				Point position(double delta) const = 0;
				Point velocity(double delta) const = 0;
				Point acceleration(double delta) const = 0;
		};

		/**
		 * A robot, as exposed by the backend.
		 */
		class Robot : public AI::Common::Robot, public Visualizable::Robot {
			public:
				/**
				 * A pointer to a Robot.
				 */
				typedef RefPtr<const Robot> Ptr;

				ObjectStore &object_store() const = 0;
				unsigned int pattern() const = 0;
				Point position(double delta) const = 0;
				double orientation(double delta) const = 0;
				Point velocity(double delta) const = 0;
				double avelocity(double delta) const = 0;
				Point acceleration(double delta) const = 0;
				double aacceleration(double delta) const = 0;
		};

		/**
		 * A player, as exposed by the backend.
		 */
		class Player : public AI::BF::W::Player, public AI::HL::W::Player, public AI::Nav::W::Player, public AI::RC::W::Player, public Visualizable::Robot {
			public:
				/**
				 * A pointer to a Player.
				 */
				typedef RefPtr<Player> Ptr;

				/**
				 * A pointer to a const Player.
				 */
				typedef RefPtr<const Player> CPtr;

				/**
				 * Returns the speeds of the four wheels as requested by the RobotController.
				 *
				 * \return the wheel speeds.
				 */
				virtual const int(&wheel_speeds() const)[4] = 0;

				Point position(double delta) const = 0;
				Point velocity(double delta) const = 0;
				Point acceleration(double delta) const = 0;
				double orientation(double delta) const = 0;
				double avelocity(double delta) const = 0;
				double aacceleration(double delta) const = 0;
				unsigned int pattern() const = 0;
				ObjectStore &object_store() const = 0;
				void move(Point dest, double ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio);
				void kick(double power);
				const std::pair<Point, double> &destination() const = 0;
				void path(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p);

			protected:
				virtual void move_impl(Point dest, double ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) = 0;
				virtual void kick_impl(double power) = 0;
				virtual void path_impl(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) = 0;
		};

		/**
		 * The friendly team.
		 */
		class FriendlyTeam : public AI::BF::W::FriendlyTeam, public AI::HL::W::FriendlyTeam, public AI::Nav::W::FriendlyTeam {
			public:
				/**
				 * Returns the size of the team.
				 *
				 * \return the size of the team.
				 */
				virtual std::size_t size() const = 0;

				/**
				 * Returns a player from the team.
				 *
				 * \param[in] i the index of the player.
				 *
				 * \return the player.
				 */
				virtual Player::Ptr get(std::size_t i) = 0;

				/**
				 * Returns a player from the team.
				 *
				 * \param[in] i the index of the player.
				 *
				 * \return the player.
				 */
				virtual Player::CPtr get(std::size_t i) const = 0;

				/**
				 * Returns the signal that is fired when a robot is added to the team.
				 *
				 * \return the signal that is fired when a robot is added to the team.
				 */
				sigc::signal<void, std::size_t> &signal_robot_added() const {
					return signal_robot_added_;
				}

				/**
				 * Returns the signal that is fired when a robot is about to be removed from the team.
				 *
				 * \return the signal that is fired when a robot is about to be removed from the team.
				 */
				sigc::signal<void, std::size_t> &signal_robot_removing() const {
					return signal_robot_removing_;
				}

				/**
				 * Returns the signal that is fired when a robot has been removed from the team.
				 *
				 * \return the signal that is fired when a robot has been removed from the team.
				 */
				sigc::signal<void> &signal_robot_removed() const {
					return signal_robot_removed_;
				}

				unsigned int score() const = 0;

			private:
				mutable sigc::signal<void, std::size_t> signal_robot_added_;
				mutable sigc::signal<void, std::size_t> signal_robot_removing_;
				mutable sigc::signal<void> signal_robot_removed_;

				AI::BF::W::Player::Ptr get_ball_filter_player(std::size_t i) const {
					return get(i);
				}

				AI::HL::W::Player::Ptr get_hl_player(std::size_t i) {
					return get(i);
				}

				AI::HL::W::Player::CPtr get_hl_player(std::size_t i) const {
					return get(i);
				}

				AI::Nav::W::Player::Ptr get_navigator_player(std::size_t i) {
					return get(i);
				}

				AI::Nav::W::Player::CPtr get_navigator_player(std::size_t i) const {
					return get(i);
				}
		};

		/**
		 * The enemy team.
		 */
		class EnemyTeam : public AI::BF::W::EnemyTeam, public AI::HL::W::EnemyTeam, public AI::Nav::W::EnemyTeam {
			public:
				/**
				 * Returns the size of the team.
				 *
				 * \return the size of the team.
				 */
				virtual std::size_t size() const = 0;

				/**
				 * Returns a robot from the team.
				 *
				 * \param[in] i the index of the robot.
				 *
				 * \return the robot.
				 */
				virtual Robot::Ptr get(std::size_t i) const = 0;

				/**
				 * Returns the signal that is fired when a robot is added to the team.
				 *
				 * \return the signal that is fired when a robot is added to the team.
				 */
				sigc::signal<void, std::size_t> &signal_robot_added() const {
					return signal_robot_added_;
				}

				/**
				 * Returns the signal that is fired when a robot is about to be removed from the team.
				 *
				 * \return the signal that is fired when a robot is about to be removed from the team.
				 */
				sigc::signal<void, std::size_t> &signal_robot_removing() const {
					return signal_robot_removing_;
				}

				/**
				 * Returns the signal that is fired when a robot has been removed from the team.
				 *
				 * \return the signal that is fired when a robot has been removed from the team.
				 */
				sigc::signal<void> &signal_robot_removed() const {
					return signal_robot_removed_;
				}

				unsigned int score() const = 0;

			private:
				mutable sigc::signal<void, std::size_t> signal_robot_added_;
				mutable sigc::signal<void, std::size_t> signal_robot_removing_;
				mutable sigc::signal<void> signal_robot_removed_;

				AI::BF::W::Robot::Ptr get_ball_filter_robot(std::size_t i) const {
					return get(i);
				}

				AI::HL::W::Robot::Ptr get_hl_robot(std::size_t i) const {
					return get(i);
				}

				AI::Nav::W::Robot::Ptr get_navigator_robot(std::size_t i) const {
					return get(i);
				}
		};

		class BackendFactory;

		/**
		 * A provider that can expose the contents of the world to the AI.
		 * A backend must get the state of the world, expose it to the AI, accept commands from the AI, and deliver those commands into the world.
		 */
		class Backend : public AI::BF::W::World, public AI::HL::W::World, public AI::Nav::W::World, public AI::RC::W::World, public Visualizable::World, public NonCopyable {
			public:
				/**
				 * The possible values indicating which end of the field the team is defending.
				 */
				enum FieldEnd {
					WEST,
					EAST,
				};

				/**
				 * The possible values indicating which colour the friendly team is.
				 */
				enum Colour {
					YELLOW,
					BLUE,
				};

				/**
				 * Returns the factory that created this Backend.
				 *
				 * \return the factory.
				 */
				virtual BackendFactory &factory() const = 0;

				/**
				 * Returns the field.
				 *
				 * \return the field.
				 */
				virtual const Field &field() const = 0;

				/**
				 * Returns the ball.
				 *
				 * \return the ball.
				 */
				virtual const Ball &ball() const = 0;

				/**
				 * Returns the friendly team.
				 *
				 * \return the friendly team.
				 */
				virtual FriendlyTeam &friendly_team() = 0;

				/**
				 * Returns the friendly team.
				 *
				 * \return the friendly team.
				 */
				virtual const FriendlyTeam &friendly_team() const = 0;

				/**
				 * Returns the enemy team.
				 *
				 * \return the enemy team.
				 */
				virtual const EnemyTeam &enemy_team() const = 0;

				/**
				 * Returns the current monotonic time.
				 * Monotonic time is a way of representing "game time", which always moves forward.
				 * Monotonic time is consistent within the game world, and may or may not be linked to real time.
				 * A navigator should \em always use this function to retrieve monotonic time, not one of the functions in util/time.h!
				 * The AI will not generally have any use for real time.
				 *
				 * \return the current monotonic time.
				 */
				virtual timespec monotonic_time() const = 0;

				/**
				 * Returns the number of table rows the backend's UI controls will consume.
				 *
				 * \return the number of rows.
				 */
				virtual unsigned int ui_controls_table_rows() const = 0;

				/**
				 * Attaches the backend's UI controls to a table.
				 *
				 * \param[in] t the table to attach to,
				 * which will have three columns,
				 * column zero being for labels,
				 * column one being for large controls,
				 * column two being for small controls,
				 * and columns two and three together being used for rows with only one control.
				 *
				 * \param[in] row the number of the first row the backend should start using.
				 */
				virtual void ui_controls_attach(Gtk::Table &t, unsigned int row) = 0;

				/**
				 * Returns or allows setting the end of the field the friendly team is defending.
				 *
				 * \return the current defending end.
				 */
				Property<FieldEnd> &defending_end() {
					return defending_end_;
				}

				/**
				 * Returns or allows setting the colour of the friendly team.
				 *
				 * \return the colour.
				 */
				Property<Colour> &friendly_colour() {
					return friendly_colour_;
				}

				/**
				 * Returns the current play type.
				 *
				 * \return the current play type.
				 */
				const Property<AI::Common::PlayType::PlayType> &playtype() const {
					return playtype_;
				}

				/**
				 * Returns or allows setting the play type override.
				 *
				 * \return the play type override.
				 */
				Property<AI::Common::PlayType::PlayType> &playtype_override() {
					return playtype_override_;
				}

				/**
				 * Returns or allows setting the ball filter being used.
				 *
				 * \return the ball filter being used.
				 */
				Property<AI::BF::BallFilter *> &ball_filter() {
					return ball_filter_;
				}

				/**
				 * Returns a signal that fires once per AI tick.
				 *
				 * \return the timer signal.
				 */
				sigc::signal<void> &signal_tick() const {
					return signal_tick_;
				}

				/**
				 * Returns a signal that fires at the very end of the AI's work each tick.
				 *
				 * \return the post-tick signal.
				 */
				sigc::signal<void> &signal_post_tick() const {
					return signal_post_tick_;
				}

				/**
				 * Returns a signal that fires when an SSL-Vision packet is received.
				 *
				 * \return the vision signal.
				 */
				sigc::signal<void, const void *, std::size_t> &signal_vision() const {
					return signal_vision_;
				}

				/**
				 * Returns a signal that fires when a referee box packet is received.
				 *
				 * \return the referee box signal.
				 */
				sigc::signal<void, const void *, std::size_t> &signal_refbox() const {
					return signal_refbox_;
				}

				/**
				 * Returns a signal that fires when the friendly or enemy team's score changes.
				 *
				 * \return the change signal.
				 */
				sigc::signal<void> &signal_score_changed() const {
					return signal_score_changed_;
				}

			protected:
				/**
				 * Constructs a new Backend.
				 */
				Backend();

				/**
				 * Destroys a Backend.
				 */
				~Backend();

				/**
				 * Allows setting the current play type.
				 *
				 * \return the current play type.
				 */
				Property<AI::Common::PlayType::PlayType> &playtype_rw() {
					return playtype_;
				}

			private:
				Property<FieldEnd> defending_end_;
				Property<Colour> friendly_colour_;
				Property<AI::Common::PlayType::PlayType> playtype_, playtype_override_;
				Property<AI::BF::BallFilter *> ball_filter_;
				mutable sigc::signal<void> signal_tick_;
				mutable sigc::signal<void> signal_post_tick_;
				mutable sigc::signal<void, const void *, std::size_t> signal_vision_;
				mutable sigc::signal<void, const void *, std::size_t> signal_refbox_;
				mutable sigc::signal<void> signal_score_changed_;
		};

		/**
		 * A factory for creating \ref Backend "Backends".
		 */
		class BackendFactory : public Registerable<BackendFactory> {
			public:
				/**
				 * Creates a new instance of the corresponding Backend and invokes a function with it.
				 *
				 * \param[in] conf the configuration file to use to configure the backend.
				 *
				 * \param[in] params the command-line parameter values to initialize the backend with.
				 *
				 * \param[in] cb a function to invoke passing the constructed Backend.
				 */
				virtual void create_backend(const Config &conf, const std::multimap<Glib::ustring, Glib::ustring> &params, sigc::slot<void, Backend &> cb) const = 0;

			protected:
				/**
				 * Constructs a new BackendFactory.
				 *
				 * \param[in] name a human-readable name for this Backend.
				 */
				BackendFactory(const char *name);

				/**
				 * Destroys a BackendFactory.
				 */
				~BackendFactory();
		};
	}
}

#endif

