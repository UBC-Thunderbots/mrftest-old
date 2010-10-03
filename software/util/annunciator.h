#ifndef UTIL_ANNUNCIATOR_H
#define UTIL_ANNUNCIATOR_H

#include "util/noncopyable.h"
#include <cstddef>
#include <glibmm.h>
#include <vector>

namespace Annunciator {
	/**
	 * A message that can be displayed in an annunciator panel.
	 * It is expected that an object that wishes to announce messages will contain one or more instances of this class as members,
	 * one for each message, and will call \c activate to turn them on and off.
	 */
	class Message : public NonCopyable, public sigc::trackable {
		public:
			/**
			 * Registers a new message source with the annunciator system.
			 *
			 * \param[in] text the text of the message.
			 */
			Message(const Glib::ustring &text);

			/**
			 * Unregisters the message source.
			 */
			~Message();

			/**
			 * The text of the message.
			 */
			const Glib::ustring text;

			/**
			 * A globally-unique ID number that refers to this message.
			 */
			const unsigned int id;

			/**
			 * Checks whether the message is active.
			 *
			 * \return \c true if the message is active, or \c false if not.
			 */
			bool active() const {
				return active_;
			}

			/**
			 * Sets whether the message is active or not.
			 *
			 * \param[in] actv \c true to activate the message, or \c false to deactivate it.
			 */
			void activate(bool actv);

			/**
			 * Returns the age of the message.
			 *
			 * \return the age of the message, in seconds (that is, the amount of time since the message was last active).
			 */
			unsigned int age() const {
				return age_;
			}

		private:
			bool active_;
			unsigned int age_;
			bool displayed_;
			sigc::connection one_second_connection;

			bool on_one_second();
			void hide();
	};

	/**
	 * Returns the currently visible (active and aging) messages.
	 *
	 * \return the messages.
	 */
	const std::vector<Message *> &visible();

	/**
	 * Fired when a hidden message is activated.
	 */
	extern sigc::signal<void> signal_message_activated;

	/**
	 * Fired when an active message is deactivated.
	 */
	extern sigc::signal<void, std::size_t> signal_message_deactivated;

	/**
	 * Fired when a message ages.
	 */
	extern sigc::signal<void, std::size_t> signal_message_aging;

	/**
	 * Fired when a still-visible but deactivated message is reactivated.
	 */
	extern sigc::signal<void, std::size_t> signal_message_reactivated;

	/**
	 * Fired when a message is hidden.
	 */
	extern sigc::signal<void, std::size_t> signal_message_hidden;

	/**
	 * Causes the audible siren to be used when annunciator messages activate.
	 */
	void activate_siren();
}

#endif

