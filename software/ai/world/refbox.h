#ifndef AI_WORLD_REFBOX_H
#define AI_WORLD_REFBOX_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <glibmm.h>

/**
 * Provides the ability to receive referee box packets.
 */
class RefBox : public NonCopyable, public sigc::trackable {
	public:
		/**
		 * Constructs a new RefBox.
		 */
		RefBox();

		/**
		 * Gets the current command character.
		 *
		 * \return the current command character.
		 */
		char command() const {
			return command_;
		}

		/**
		 * Fired when the current command character changes.
		 */
		sigc::signal<void> signal_command_changed;

		/**
		 * Gets the blue team's score.
		 *
		 * \return the current number of goals awarded to the blue team.
		 */
		unsigned int goals_blue() const {
			return goals_blue_;
		}

		/**
		 * Gets the yellow team's score.
		 *
		 * \return the current number of goals awarded to the yellow team.
		 */
		unsigned int goals_yellow() const {
			return goals_yellow_;
		}

		/**
		 * Gets the time remaining in the current game stage.
		 *
		 * \return the number of seconds remaining in the current game stage.
		 */
		unsigned int time_remaining() const {
			return time_remaining_;
		}

	private:
		const FileDescriptor::Ptr fd;
		char command_;
		unsigned char goals_blue_, goals_yellow_;
		unsigned short time_remaining_;

		bool on_readable(Glib::IOCondition);
};

#endif

