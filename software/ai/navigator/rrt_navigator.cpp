#include "ai/navigator/rrt_navigator.h"
#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "ai/hl/stp/param.h"
#include "ai/navigator/util.h"
#include <iostream>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;
using AI::BE::Primitives::Primitive;
using AI::BE::Primitives::PrimitiveDescriptor;

namespace AI {
	namespace Nav {
		namespace RRT {
			IntParam jon_hysteris_hack(u8"Jon Hysteris Hack", u8"AI/Nav/RRT", 2, 1, 10);

			DoubleParam angle_increment(u8"angle increment (deg)", u8"AI/Nav/RRT", 10, 1, 100);
			DoubleParam linear_increment(u8"linear increment (m)", u8"AI/Nav/RRT", 0.05, 0.001, 1);

			DoubleParam default_desired_rpm(u8"The default desired rpm for dribbling", u8"AI/Movement/Primitives", 7000, 0, 100000);

			class RRTNavigator final : public Navigator {
				public:
					explicit RRTNavigator(World world);
					void tick() override;
					void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override;
					NavigatorFactory &factory() const override;
				private:
					void plan(Player player);
					RRTPlanner planner;
			};
		}
	}
}

using AI::Nav::RRT::RRTNavigator;

RRTNavigator::RRTNavigator(AI::Nav::W::World world) : Navigator(world), planner(world) {
}

void RRTNavigator::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
	ctx->set_source_rgb(1.0, 1.0, 1.0);

	for (const Player player : world.friendly_team()) {
		if (!std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)])) {
			player.object_store()[typeid(*this)] = std::make_shared<PlayerData>();
		}

		PlayerData::Ptr player_data = std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)]);

		if (has_destination(player_data->hl_request)) {
			Point dest = player_data->hl_request.field_point();
			Angle angle = player_data->hl_request.field_angle();
			ctx->set_source_rgb(1, 1, 1);
			ctx->begin_new_path();
			ctx->arc(dest.x, dest.y, 0.09, angle.to_radians() + M_PI_4, angle.to_radians() - M_PI_4);
			ctx->stroke();
		}
	}
}

void RRTNavigator::plan(Player player) {
}

void RRTNavigator::tick() {
	for (Player player : world.friendly_team()) {
		plan(player);
	}
}

NAVIGATOR_REGISTER(RRTNavigator)
