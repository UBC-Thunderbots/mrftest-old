#ifndef AI_BACKEND_XBEE_REFBOX_H
#define AI_BACKEND_XBEE_REFBOX_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <glibmm.h>

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
					 */
					RefBox();

				private:
					const FileDescriptor::Ptr fd;

					bool on_readable(Glib::IOCondition);
			};
		}
	}
}

#endif

