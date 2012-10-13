#ifndef AI_BACKEND_BACKEND_H
#define AI_BACKEND_BACKEND_H

#include "ai/backend/ball.h"
#include "ai/backend/field.h"
#include "ai/backend/player.h"
#include "ai/backend/robot.h"
#include "ai/backend/team.h"
#include "ai/common/team.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "uicomponents/visualizer.h"
#include "util/box_ptr.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include "util/predictor.h"
#include "util/property.h"
#include "util/registerable.h"
#include <functional>
#include <gtkmm/table.h>
#include <sigc++/signal.h>

namespace Gtk {
	class Widget;
}

namespace AI {
	namespace BF {
		class BallFilter;
	}

	namespace BE {
		/**
		 * \brief The system loop delay, in seconds
		 */
		extern DoubleParam LOOP_DELAY;

		class BackendFactory;

		/**
		 * \brief A provider that can expose the contents of the world to the AI.
		 *
		 * A backend must get the state of the world, expose it to the AI, accept commands from the AI, and deliver those commands into the world.
		 */
		class Backend : public Visualizable::World, public NonCopyable {
			public:
				/**
				 * \brief The possible values indicating which end of the field the team is defending.
				 */
				enum class FieldEnd {
					WEST,
					EAST,
				};

				/**
				 * \brief Returns the factory that created this Backend.
				 *
				 * \return the factory.
				 */
				virtual BackendFactory &factory() const = 0;

				/**
				 * \brief Returns the field.
				 *
				 * \return the field.
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
				virtual const Team<Player> &friendly_team() const = 0;

				/**
				 * \brief Returns the enemy team
				 *
				 * \return the enemy team
				 */
				virtual const Team<Robot> &enemy_team() const = 0;

				/**
				 * \brief Returns the current monotonic time.
				 *
				 * Monotonic time is a way of representing "game time", which always moves forward.
				 * Monotonic time is consistent within the game world, and may or may not be linked to real time.
				 * A navigator should \em always use this function to retrieve monotonic time, not one of the functions in util/time.h!
				 * The AI will not generally have any use for real time.
				 *
				 * \return the current monotonic time.
				 */
				timespec monotonic_time() const;

				/**
				 * \brief Returns the number of table rows the backend's main tab UI controls will consume.
				 *
				 * \return the number of rows.
				 */
				virtual unsigned int main_ui_controls_table_rows() const = 0;

				/**
				 * \brief Attaches the backend's main tab UI controls to a table.
				 *
				 * \param[in] t the table to attach to,
				 * which will have three columns,
				 * column zero being for labels,
				 * column one being for large controls,
				 * column two being for small controls,
				 * and columns two and three together being used for rows with only one control or multiple equal-sized controls.
				 *
				 * \param[in] row the number of the first row the backend should start using.
				 */
				virtual void main_ui_controls_attach(Gtk::Table &t, unsigned int row) = 0;

				/**
				 * \brief Returns the number of table rows the backend's secondary tab UI controls will consume.
				 *
				 * \return the number of rows.
				 */
				virtual unsigned int secondary_ui_controls_table_rows() const = 0;

				/**
				 * \brief Attaches the backend's secondary tab UI controls to a table.
				 *
				 * \param[in] t the table to attach to,
				 * which will have three columns,
				 * column zero being for labels,
				 * column one being for large controls,
				 * column two being for small controls,
				 * and columns two and three together being used for rows with only one control or multiple equal-sized controls.
				 *
				 * \param[in] row the number of the first row the backend should start using.
				 */
				virtual void secondary_ui_controls_attach(Gtk::Table &t, unsigned int row) = 0;

				/**
				 * \brief Returns or allows setting the end of the field the friendly team is defending.
				 *
				 * \return the current defending end.
				 */
				Property<FieldEnd> &defending_end() {
					return defending_end_;
				}

				/**
				 * \brief Returns or allows setting the colour of the friendly team.
				 *
				 * \return the colour.
				 */
				Property<AI::Common::Colour> &friendly_colour() {
					return friendly_colour_;
				}

				/**
				 * \brief Returns the current play type.
				 *
				 * \return the current play type.
				 */
				const Property<AI::Common::PlayType> &playtype() const {
					return playtype_;
				}

				/**
				 * \brief Returns or allows setting the play type override.
				 *
				 * \return the play type override.
				 */
				Property<AI::Common::PlayType> &playtype_override() {
					return playtype_override_;
				}

				/**
				 * \brief Returns or allows setting the ball filter being used.
				 *
				 * \return the ball filter being used.
				 */
				Property<AI::BF::BallFilter *> &ball_filter() {
					return ball_filter_;
				}

				/**
				 * \brief Returns a signal that fires once per AI tick.
				 *
				 * \return the timer signal.
				 */
				sigc::signal<void> &signal_tick() const {
					return signal_tick_;
				}

				/**
				 * \brief Returns a signal that fires at the very end of the AI's work each tick.
				 *
				 * \return the post-tick signal.
				 */
				sigc::signal<void, unsigned int> &signal_post_tick() const {
					return signal_post_tick_;
				}

				/**
				 * \brief Returns a signal that fires when an SSL-Vision packet is received.
				 *
				 * \return the vision signal.
				 */
				sigc::signal<void, timespec, const SSL_WrapperPacket &> &signal_vision() const {
					return signal_vision_;
				}

				/**
				 * \brief Returns a signal that fires when a referee box packet is received.
				 *
				 * \return the referee box signal.
				 */
				sigc::signal<void, timespec, const void *, std::size_t> &signal_refbox() const {
					return signal_refbox_;
				}

				/**
				 * \brief Returns a signal that fires when the visualizer needs an overlay to be drawn.
				 *
				 * \return the signal.
				 */
				sigc::signal<void, Cairo::RefPtr<Cairo::Context> > &signal_draw_overlay() const {
					return signal_draw_overlay_;
				}

			protected:
				/**
				 * \brief The field
				 */
				Field field_;

				/**
				 * \brief The ball
				 */
				Ball ball_;

				/**
				 * \brief The current monotonic time
				 */
				timespec monotonic_time_;

				/**
				 * \brief Constructs a new Backend.
				 */
				explicit Backend();

				/**
				 * \brief Allows setting the current play type.
				 *
				 * \return the current play type.
				 */
				Property<AI::Common::PlayType> &playtype_rw() {
					return playtype_;
				}

			private:
				Property<FieldEnd> defending_end_;
				Property<AI::Common::Colour> friendly_colour_;
				Property<AI::Common::PlayType> playtype_, playtype_override_;
				Property<AI::BF::BallFilter *> ball_filter_;
				mutable sigc::signal<void> signal_tick_;
				mutable sigc::signal<void, unsigned int> signal_post_tick_;
				mutable sigc::signal<void, timespec, const SSL_WrapperPacket &> signal_vision_;
				mutable sigc::signal<void, timespec, const void *, std::size_t> signal_refbox_;
				mutable sigc::signal<void, Cairo::RefPtr<Cairo::Context> > signal_draw_overlay_;

				void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
		};

		/**
		 * \brief A factory for creating \ref Backend "Backends".
		 */
		class BackendFactory : public Registerable<BackendFactory> {
			public:
				/**
				 * \brief Creates a new instance of the corresponding Backend and invokes a function with it.
				 *
				 * \param[in] load_filename the filename of a simulator state file to restore.
				 *
				 * \param[in] camera_mask a bitmask of cameras whose data should be accepted.
				 *
				 * \param[in] multicast_interface the index of the interface on which to join multicast groups, or zero to use the kernel's default choice.
				 *
				 * \param[in] cb a function to invoke passing the constructed Backend.
				 */
				virtual void create_backend(const std::string &load_filename, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const = 0;

			protected:
				/**
				 * \brief Constructs a new BackendFactory.
				 *
				 * \param[in] name a human-readable name for this Backend.
				 */
				explicit BackendFactory(const char *name);
		};
	}
}



inline const AI::BE::Field &AI::BE::Backend::field() const {
	return field_;
}

inline const AI::BE::Ball &AI::BE::Backend::ball() const {
	return ball_;
}

inline timespec AI::BE::Backend::monotonic_time() const {
	return monotonic_time_;
}

#endif

