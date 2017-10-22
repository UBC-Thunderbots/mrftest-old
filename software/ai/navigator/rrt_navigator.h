#pragma once

#include <memory>
#include "ai/backend/player.h"
#include "ai/backend/primitives/primitive.h"
#include "ai/flags.h"
#include "ai/util.h"
#include "util/object_store.h"

namespace AI
{
namespace Nav
{
namespace RRT
{
class PlayerData final : public ObjectStore::Element
{
   public:
    inline PlayerData()
    {
    }

    typedef std::shared_ptr<PlayerData> Ptr;
    AI::Flags::MovePrio prev_move_prio;
    AI::Flags::AvoidDistance prev_avoid_distance;
    Point previous_dest;
    Angle previous_orient;

    AI::BE::Primitives::PrimitiveDescriptor last_shoot_primitive;
    AI::BE::Primitives::PrimitiveDescriptor hl_request;
    AI::BE::Primitives::PrimitiveDescriptor nav_request;
    bool fancy_shoot_maneuver;
};
}
}
}
