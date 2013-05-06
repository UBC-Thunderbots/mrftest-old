#ifndef AI_BACKEND_REFBOX_H
#define AI_BACKEND_REFBOX_H

#include "proto/referee.pb.h"
#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <glibmm/main.h>

namespace AI {
	namespace BE {
		/**
		 * \brief Provides the ability to receive referee box packets.
		 */
		class RefBox : public NonCopyable, public sigc::trackable {
			public:
				/**
				 * \brief The most recently received referee box packet.
				 */
				SSL_Referee packet;

				/**
				 * Fired on receipt of a packet.
				 */
				mutable sigc::signal<void> signal_packet;

				/**
				 * Constructs a new RefBox.
				 *
				 * \param[in] multicast_interface the index of the interface on which to join the multicast group, or zero to use the kernel's default choice.
				 */
				explicit RefBox(int multicast_interface);

			private:
				const FileDescriptor fd;

				bool on_readable(Glib::IOCondition);
		};
	}
}

#endif

