#ifndef AI_WORLD_WORLD_H
#define AI_WORLD_WORLD_H

#include "ai/ball_filter/ball_filter.h"
#include "ai/world/ball.h"
#include "ai/world/field.h"
#include "ai/world/player.h"
#include "ai/world/playtype.h"
#include "ai/world/refbox.h"
#include "ai/world/team.h"
#include "uicomponents/visualizer.h"
#include "util/clocksource.h"
#include "util/config.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "xbee/client/drive.h"
#include <stdint.h>
#include <vector>
#include <sigc++/sigc++.h>

namespace AI {
	class Window;

	/**
	 * Collects all information the AI needs to examine the state of the world and transmit orders to robots.
	 */
	class World : public NonCopyable {
		public:
			/**
			 * The configuration file.
			 */
			const Config &conf;

			/**
			 * Fired when a detection packet is received.
			 */
			sigc::signal<void> signal_detection;

			/**
			 * Fired when a field geometry packet is received.
			 */
			sigc::signal<void> signal_geometry;

			/**
			 * Fired when the play type changes.
			 */
			sigc::signal<void> signal_playtype_changed;

			/**
			 * Fired when the current team switches ends.
			 */
			sigc::signal<void> signal_flipped_ends;

			/**
			 * Fired when the local team's RobotColour with respect to the referee box changes.
			 */
			sigc::signal<void> signal_flipped_refbox_colour;

			/**
			 * The friendly team.
			 */
			FriendlyTeam friendly;

			/**
			 * The enemy team.
			 */
			EnemyTeam enemy;

			/**
			 * Creates a new World object.
			 *
			 * \param[in] conf the configuration file.
			 *
			 * \param[in] xbee_bots the robots to drive.
			 */
			World(const Config &conf, const std::vector<XBeeDriveBot::Ptr> &xbee_bots);

			/**
			 * Gets the ball.
			 *
			 * \return the Ball.
			 */
			Ball::Ptr ball() const {
				return ball_;
			}

			/**
			 * Gets the field.
			 *
			 * \return the Field.
			 */
			const Field &field() const {
				return field_;
			}

			/**
			 * Gets the end of the physical field the friendly team is defending.
			 * 
			 * \return \c true if the friendly team is defending the "east" end of the field
			 * (the end Which end whose SSL-Vision coordinate are positive),
			 * or \c false if the friendly team is defending the "west" end.
			 */
			bool east() const {
				return east_;
			}

			/**
			 * Inverts which end of the physical field the local team is defending.
			 */
			void flip_ends();

			/**
			 * Gets the colour of the friendly team for the purpose of referee box commands.
			 *
			 * \return \c true if the friendly team is yellow, or \c false if it is blue.
			 */
			bool refbox_yellow() const {
				return refbox_yellow_;
			}

			/**
			 * Inverts which colour the local team should be considered to be with respect to referee box commands.
			 */
			void flip_refbox_colour();

			/**
			 * Gets the current state of play.
			 *
			 * \return the current state of play.
			 */
			PlayType::PlayType playtype() const {
				return playtype_;
			}

			/**
			 * Gets a view of the world for a Visualizer.
			 *
			 * \return a Visualizable view of the world.
			 */
			const Visualizable &visualizer_view() const {
				return vis_view;
			}

			/**
			 * Gets the current ball filter.
			 *
			 * \return the currently-active ball filter.
			 */
			BallFilter *ball_filter() const {
				return ball_filter_;
			}

			/**
			 * Sets which ball filter should be used.
			 *
			 * \param filter the new filter to use.
			 */
			void ball_filter(BallFilter *filter);

			/**
			 * Gets the elapsed time since program startup.
			 *
			 * \return the elapsed time, in AI ticks.
			 */
			uint64_t timestamp() const {
				return timestamp_;
			}

			/**
			 * Increments the timestamp.
			 */
			void tick_timestamp();

			/**
			 * Gets the length of time the current play type has been active.
			 *
			 * \return the time in seconds.
			 */
			double playtype_time() const;

		private:
			class VisualizerView : public Visualizable {
				public:
					VisualizerView(const World * const w) : world(w) {
					}

					const Visualizable::Field &field() const {
						return world->field();
					}

					Visualizable::Ball::Ptr ball() const {
						return world->ball();
					}

					std::size_t size() const {
						return world->friendly.size() + world->enemy.size();
					}

					Visualizable::Robot::Ptr operator[](unsigned int index) const {
						if (index < world->friendly.size()) {
							return world->friendly.get_robot(index);
						} else {
							return world->enemy.get_robot(index - world->friendly.size());
						}
					}

				private:
					const World * const world;
			};

			bool east_;
			bool refbox_yellow_;
			const FileDescriptor::Ptr vision_socket;
			RefBox refbox_;
			Field field_;
			Ball::Ptr ball_;
			SSL_DetectionFrame detections[2];
			const std::vector<XBeeDriveBot::Ptr> xbee_bots;
			PlayType::PlayType playtype_;
			PlayType::PlayType playtype_override;
			bool playtype_override_active;
			VisualizerView vis_view;
			BallFilter *ball_filter_;
			Point playtype_arm_ball_position;
			uint64_t timestamp_;
			timespec playtype_time_;

			bool on_vision_readable(Glib::IOCondition);
			void override_playtype(PlayType::PlayType);
			void clear_playtype_override();
			void update_playtype();
			PlayType::PlayType compute_playtype(PlayType::PlayType);

			friend class Window;
	};
}

#endif

