#ifndef AI_BACKEND_XBEE_REFBOX_H
#define AI_BACKEND_XBEE_REFBOX_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <glibmm/main.h>

namespace AI {
	namespace BE {
		namespace XBee {
			/**
			 * Provides the ability to receive referee box packets.
			 */
			class RefBox : public NonCopyable, public sigc::trackable {
				public:
					/**
					 * The current command character.
					 */
					Property<char> command;

					/**
					 * The blue team's score.
					 */
					Property<unsigned int> goals_blue;

					/**
					 * The yellow team's score.
					 */
					Property<unsigned int> goals_yellow;

					/**
					 * Fired on receipt of a packet.
					 */
					mutable sigc::signal<void, const void *, std::size_t> signal_packet;

					/**
					 * Constructs a new RefBox.
					 *
					 * \param[in] multicast_interface the index of the interface on which to join the multicast group, or zero to use the kernel's default choice.
					 */
					explicit RefBox(unsigned int multicast_interface);

				private:
					const FileDescriptor fd;

					bool on_readable(Glib::IOCondition);
			};
		}
	}
}

#endif

