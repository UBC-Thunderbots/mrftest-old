#ifndef AI_NAVIGATOR_RRT_NAVIGATOR_H
#define AI_NAVIGATOR_RRT_NAVIGATOR_H

#include "ai/param.h"
#include "ai/util.h"
#include "util/object_store.h"
#include <memory>
#include "ai/flags.h"

namespace AI {
	namespace Nav {
		namespace RRT {

			class PlayerData : public ObjectStore::Element {
				public:
					typedef std::shared_ptr<PlayerData> Ptr;
					unsigned int added_flags;
					Point previous_point;

					AI::Flags::MoveType prev_move_type;
					AI::Flags::MovePrio prev_move_prio;
					AI::Flags::AvoidDistance prev_avoid_distance;
					Point previous_dest;
			};
		}
	}
}

#endif
