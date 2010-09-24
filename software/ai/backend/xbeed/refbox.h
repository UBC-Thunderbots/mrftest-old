#ifndef AI_BACKEND_XBEED_REFBOX_H
#define AI_BACKEND_XBEED_REFBOX_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <glibmm.h>

namespace AI {
	namespace BE {
		namespace XBeeD {
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
					 * Constructs a new RefBox.
					 */
					RefBox();

					/**
					 * Destroys a RefBox.
					 */
					~RefBox();

				private:
					const FileDescriptor::Ptr fd;

					bool on_readable(Glib::IOCondition);
			};
		}
	}
}

#endif

