#pragma once

#include "ai/backend/primitives/primitive.h"
#include "ai/util.h"
#include "util/object_store.h"
#include "ai/backend/player.h"
#include <memory>
#include "ai/flags.h"

namespace AI {
	namespace Nav {
		namespace RRT {
			class PlayerData final : public ObjectStore::Element {
				public:
					inline PlayerData() { }
			};
		}
	}
}
