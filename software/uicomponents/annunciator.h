#ifndef UICOMPONENTS_ANNUNCIATOR_H
#define UICOMPONENTS_ANNUNCIATOR_H

#include "util/noncopyable.h"
#include <gtkmm.h>

/**
 * An annunciator panel that displays a collection of important messages.
 */
class annunciator : public Gtk::ScrolledWindow {
	public:
		/**
		 * A message that can be displayed in the annunciator panel. It is
		 * expected that an object that wishes to announce messages will contain
		 * one or more instances of this class as members, one for each message,
		 * and will call \c activate to turn them on and off.
		 */
		class message : public noncopyable {
			public:
				/**
				 * Registers a new message source with the annunciator system.
				 *
				 * \param[in] text the text of the message.
				 */
				message(const Glib::ustring &text);

				/**
				 * Unregisters the message source.
				 */
				~message();

				/**
				 * The text of the message.
				 */
				const Glib::ustring text;

				/**
				 * A globally-unique ID number that refers to this message.
				 */
				const unsigned int id;

				/**
				 * \return \c true if the message is active, or \c false if not.
				 */
				bool active() const {
					return active_;
				}

				/**
				 * Sets whether the message is active or not.
				 *
				 * \param[in] actv \c true to activate the message, or \c false
				 * to deactivate it.
				 */
				void activate(bool actv);

			private:
				bool active_;
		};

		/**
		 * Constructs a new annunciator panel ready to add to a window.
		 */
		annunciator();

		/**
		 * Destroys an annunciator panel.
		 */
		~annunciator();
};

#endif

