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
#include "util/byref.h"
#include "util/clocksource.h"
#include "util/config.h"
#include "util/fd.h"
#include "xbee/client/drive.h"
#include <stdint.h>
#include <vector>
#include <sigc++/sigc++.h>

class AIWindow;

/**
 * Collects all information the AI needs to examine the state of the world and
 * transmit orders to robots.
 */
class World : public ByRef {
	public:
		/**
		 * A pointer to a World object.
		 */
		typedef RefPtr<World> Ptr;

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
		 * Fired when the local team's RobotColour with respect to the referee box
		 * changes.
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
		 * \param conf the configuration file
		 * \param xbee_bots the robots to drive
		 * \return The new object
		 */
		static Ptr create(const Config &conf, const std::vector<XBeeDriveBot::Ptr> &xbee_bots);

		/**
		 * \return The ball
		 */
		Ball::Ptr ball() const {
			return ball_;
		}

		/**
		 * \return The Field
		 */
		const Field &field() const {
			return field_;
		}

		/**
		 * \return Which end of the physical field the local team is defending
		 */
		bool east() const {
			return east_;
		}

		/**
		 * Inverts which end of the physical field the local team is defending.
		 */
		void flip_ends();

		/**
		 * \return Which colour the local team considers itself to be with
		 * respect to referee box commands
		 */
		bool refbox_yellow() const {
			return refbox_yellow_;
		}

		/**
		 * Inverts which colour the local team should be considered to be with
		 * respect to referee box commands.
		 */
		void flip_refbox_colour();

		/**
		 * \return The current state of play
		 */
		PlayType::PlayType playtype() const {
			return playtype_;
		}

		/**
		 * \return A Visualizable view of the world
		 */
		const Visualizable &visualizer_view() const {
			return vis_view;
		}

		/**
		 * \return The currently-active ball filter
		 */
		BallFilter *ball_filter() const {
			return ball_filter_;
		}

		/**
		 * Sets which ball filter should be used.
		 *
		 * \param filter the new filter to use
		 */
		void ball_filter(BallFilter *filter);

		/**
		 * \return the number of AI ticks that have occurred since program
		 * startup.
		 */
		uint64_t timestamp() const {
			return timestamp_;
		}

		/**
		 * Increments the timestamp.
		 */
		void tick_timestamp();

		/**
		 * Playtype time.
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
		const FileDescriptor vision_socket;
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

		World(const Config &, const std::vector<XBeeDriveBot::Ptr> &);
		bool on_vision_readable(Glib::IOCondition);
		void override_playtype(PlayType::PlayType);
		void clear_playtype_override();
		void update_playtype();
		PlayType::PlayType compute_playtype(PlayType::PlayType);

		friend class AIWindow;
};

#endif

