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
					enum ShootActionType
					{
						NO_ACTION_OR_PIVOT = 0,
						SHOULD_SHOOT,
						SHOULD_MOVE,
						SHOULD_PIVOT,
						NO_ACTION_OR_MOVE,
						SHOOT_FAILED
					};

					ShootActionType shoot_movement_zoning(Player player, Point ball_pos, Point target_pos);
					ShootActionType shoot_movement_sequence(Player player, Point ball_pos, Point target_pos, PrimitiveDescriptor last_primitive);
					PrimitiveDescriptor move_in_local_coord(Player player, PrimitiveDescriptor global);
					PrimitiveDescriptor pivot_in_local_coord(Player player, PrimitiveDescriptor shoot_pivot_request);
					RRTPlanner planner;
			};
		}
	}
}

using AI::Nav::RRT::RRTNavigator;

RRTNavigator::RRTNavigator(AI::Nav::W::World world) : Navigator(world), planner(world) {
}

RRTNavigator::ShootActionType RRTNavigator::shoot_movement_sequence(Player player, Point ball_pos, Point target_pos, PrimitiveDescriptor last_primitive) {
	ShootActionType zone = shoot_movement_zoning(player, ball_pos, target_pos);
	if (last_primitive.prim == Drive::Primitive::SHOOT) {
		if (zone != SHOOT_FAILED ){
			return SHOULD_SHOOT;
		}
		else {
			return SHOULD_PIVOT;
		}
	}
	else if (last_primitive.prim == Drive::Primitive::PIVOT) {
		if (zone != SHOULD_SHOOT) {
			return SHOULD_PIVOT;
		}
		else {
			return SHOULD_SHOOT;
		}
	}
	else {
		return zone;
	}
}

RRTNavigator::ShootActionType RRTNavigator::shoot_movement_zoning(Player player, Point ball_pos, Point target_pos) {
	Point ball2target = target_pos - ball_pos;
	Point ball2player = player.position() - ball_pos;
	Angle swing_diff = (ball2target.orientation() - ball2player.orientation() + Angle::half()).angle_mod();

	if (swing_diff.abs() <= Angle::of_degrees(5)) {
		// within angle tolerance of pivot
		return SHOULD_SHOOT;
	} else if (swing_diff.abs() <= Angle::of_degrees(90) && ball2player.len()
		<= Robot::MAX_RADIUS)
	{
		if (std::fabs(ball2player.dot(ball2target)) <= 0.3 * Robot::MAX_RADIUS) {	
			// within distance tolerance to shoot ball, but geometry is really off
			return SHOOT_FAILED;
		}
		else {
			// within distance tolerance to shoot ball
			return SHOULD_SHOOT;
		}
	}
	else if (swing_diff.abs() > Angle::of_degrees(5) && swing_diff.abs() <= Angle::of_degrees(50)) {
		// boundary between pivot, shoot and move
		return NO_ACTION_OR_PIVOT;
	}
	else if (ball2player.len() < 1.0) {
		// close to ball but angle is too off
		return SHOULD_PIVOT;
	}
	else if (ball2player.len() < 1.4) {
		// Boundary between pivot and move
		return NO_ACTION_OR_MOVE;
	}
	else {
		// Get further away so we can pivot safely.
		return SHOULD_MOVE;
	}
}


PrimitiveDescriptor RRTNavigator::pivot_in_local_coord(Player player, PrimitiveDescriptor global) {
	Point player2centre = global.field_point() - player.position();
	Angle pivot_swing = (global.field_angle() - player2centre.orientation() );
	Point pivot_dest_local = player2centre.rotate(-player.orientation());
	Angle rotate_angle_local = global.field_angle() - player.orientation();

	PrimitiveDescriptor local(global.prim, pivot_dest_local.x, pivot_dest_local.y, pivot_swing.to_radians(), rotate_angle_local.to_radians(), 0);
	return local;
}

PrimitiveDescriptor RRTNavigator::move_in_local_coord(Player player, PrimitiveDescriptor global) {
	Point pos_diff = global.field_point() - player.position();
	Point robot_local_dest = pos_diff.rotate(-player.orientation());
	Angle robot_local_angle = (global.field_angle() - player.orientation());

	if ((player.flags() & AI::Flags::MoveFlags::CAREFUL) != AI::Flags::MoveFlags::NONE) {
#warning Robocup 2016 this is a hack to limit velocity on the controller side! but it seems to work well...
		Angle sign = robot_local_angle.to_degrees() > 0 ? Angle::of_degrees(angle_increment) : Angle::of_degrees(-angle_increment);

		if (sign.abs() < (global.field_angle() - player.orientation()).abs()) {
			robot_local_angle = sign;
		}

		if (robot_local_dest.len() > linear_increment) {
			robot_local_dest = robot_local_dest.norm() * linear_increment;
		}
	}

	PrimitiveDescriptor local(global.prim, robot_local_dest.x, robot_local_dest.y, robot_local_angle.to_radians(), 0, 0);

	return local;
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
	if (!std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)])) {
		player.object_store()[typeid(*this)] = std::make_shared<PlayerData>();
	}

	PlayerData::Ptr player_data = std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)]);

	PrimitiveDescriptor hl_request(Drive::Primitive::STOP, 0, 0, 0, 0, 1);

	if (player.has_prim()) {
		hl_request = player.top_prim()->desc();
		player.top_prim()->active(true);
	}

	// just a hack for now, defense logic should be implemented somewhere else
	// positive x is enemy goal
	double x_limit = world.field().enemy_goal().x - world.field().defense_area_radius() / 2;
	double y_limit = (world.field().defense_area_stretch() + world.field().defense_area_radius() * 2) / 2;

	bool defense_area_violation =
		hl_request.field_point().x > x_limit &&
		std::fabs(hl_request.field_point().y) < y_limit &&
		((player.flags() & AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE) != AI::Flags::MoveFlags::NONE);

	// starting a primitive
	PrimitiveDescriptor nav_request = hl_request;
	PrimitiveDescriptor nav_dest = hl_request;

	switch (hl_request.prim) {
		case Drive::Primitive::STOP:
			// No planning.
			break;
		case Drive::Primitive::MOVE:
		case Drive::Primitive::DRIBBLE:
		case Drive::Primitive::SHOOT:
		case Drive::Primitive::SPIN:
			// These all try to move to a target position. If we can’t
			// get there, do an RRT plan and MOVE to the next path
			// point instead.
			if (!valid_path(player.position(), hl_request.field_point(), world, player)) {
#warning Do we need flags here, e.g. to let the goalie into the defense area?
				const std::vector<Point> &plan = planner.plan(player, hl_request.field_point(), AI::Flags::MoveFlags::NONE);

				if (!plan.empty()) {
					nav_request.params[0] = plan[0].x;
					nav_request.params[1] = plan[0].y;

					nav_dest.params[0] = plan.back().x;
					nav_dest.params[1] = plan.back().y;
				}

				player.display_path(plan);
			}
			break;

		case Drive::Primitive::CATCH:
#warning Check how to do clear path checking and RRT planning during catch.
			break;
		case Drive::Primitive::PIVOT:
#warning Check how to do clear path checking and RRT planning during pivot.
			break;
		case Drive::Primitive::DIRECT_WHEELS:
		case Drive::Primitive::DIRECT_VELOCITY:
			// The AI should not use these.
			std::abort();
	}

	double hl_request_len = hl_request.field_point().len();
	double nav_request_len = nav_request.field_point().len();

	/*
	TODO do done stuff
	if (is_done(player, nav_dest) && player.has_prim()) {
		// stop primitive and continue with next one
		player.top_prim()->active(false);
		player.top_prim()->done(true);

		return;
	}
	*/

	player_data->hl_request = hl_request;
	player_data->nav_request = nav_request;

	// execute the plan
	// some calculation that are generally useful
	Point pos_diff = nav_request.field_point() - player.position();
	Point robot_local_dest = pos_diff.rotate(-player.orientation());

	// some shoot related stuff
	ShootActionType zone;

	PrimitiveDescriptor pivot_request;
	PrimitiveDescriptor pivot_local;
	PrimitiveDescriptor shoot_request;
	PrimitiveDescriptor local_coord;
	Point target_position = Point::of_angle(hl_request.field_angle()) + hl_request.field_point();

	if (hl_request.prim != Drive::Primitive::SHOOT) {
		player_data->last_shoot_primitive.prim = Drive::Primitive::MOVE;
	}

/*
	switch (nav_request.prim) {
		case Drive::Primitive::STOP:
			if (nav_request.extra & 1) {
				player.move_brake();
			}
			else {
				player.move_coast();
			}
			break;
		case Drive::Primitive::MOVE:
			if (nav_request.extra & 1) { // care angle
				local_coord = move_in_local_coord(player, nav_request);
				LOG_DEBUG(Glib::ustring::compose("Time for new move robot %3, point %1, angle %2", local_coord.field_point(), local_coord.field_angle(), player.pattern()));

				player.move_move(local_coord.field_point(), local_coord.field_angle(), hl_request.params[3] * nav_request_len / hl_request_len);
			}
			else {
				nav_request.field_angle() = Angle(); // fill in angle so the function doesn't crash
				local_coord = move_in_local_coord(player, nav_request);
				LOG_DEBUG(Glib::ustring::compose("Time for new move, point %1", local_coord.field_point()));
				player.move_move(local_coord.field_point(), local_coord.field_angle(), hl_request.params[3] * nav_request_len / hl_request_len);
			}
			break;
		case Drive::Primitive::DRIBBLE:
			local_coord = move_in_local_coord(player, nav_request);
			LOG_DEBUG(Glib::ustring::compose("Time for new dribble point %1, angle %2", local_coord.field_point(), local_coord.field_angle()));
			player.move_dribble(local_coord.field_point(), local_coord.field_angle(), default_desired_rpm, false);
			break;
		case Drive::Primitive::SHOOT:
			if (nav_request.extra & 2) { // care angle
				// evaluate if we need to do a maneuver
				player_data->fancy_shoot_maneuver = false;
				// evaluate whether we are facing the right way to kick on target
				zone = shoot_movement_sequence(player, nav_request.field_point(), target_position, player_data->last_shoot_primitive);
				switch (zone) {
					case SHOULD_SHOOT: // shooting
						LOG_DEBUG(Glib::ustring::compose("new shoot first time (pos %1, angle %2)", hl_request.field_point(), hl_request.field_angle()));
						local_coord = move_in_local_coord(player, hl_request);
						shoot_request = hl_request;
						player.move_shoot(local_coord.field_point(), local_coord.field_angle(), hl_request.params[3], hl_request.extra & 1);
						LOG_DEBUG(Glib::ustring::compose("local params for shoot (pos %1, angle %2)", local_coord.field_point(), local_coord.field_angle()));
						player_data->last_shoot_primitive = shoot_request;
						break;
					case SHOULD_MOVE: // moves a little closer to pivot center
					case NO_ACTION_OR_MOVE:
						LOG_DEBUG(Glib::ustring::compose("new move first time (pos %1, angle %2)", hl_request.field_point(), hl_request.field_angle()));
						local_coord = move_in_local_coord(player, hl_request);
						player.move_move(local_coord.field_point(), local_coord.field_angle(), hl_request.params[3] * nav_request_len / hl_request_len);
						LOG_DEBUG(Glib::ustring::compose("local params for move, (pos %1, angle %2)", local_coord.field_point(), local_coord.field_angle()));
						shoot_request = hl_request;
						shoot_request.prim = Drive::Primitive::MOVE;
						player_data->last_shoot_primitive = shoot_request;
						break;
					case NO_ACTION_OR_PIVOT: // when in doubt, do pivot
					case SHOULD_PIVOT: // pivot around ball
						LOG_DEBUG(Glib::ustring::compose("new pivot first time (pos %1, angle %2)", hl_request.field_point(), hl_request.field_angle()));
						pivot_request = hl_request;
						pivot_request.prim = Drive::Primitive::PIVOT;
						pivot_local = pivot_in_local_coord( player, pivot_request );
						player.move_pivot(pivot_local.field_point(), pivot_local.field_angle(), pivot_local.field_angle());
						LOG_DEBUG(Glib::ustring::compose("local params for pivot, center %1, swing %2, rotation %3",
									pivot_local.field_point(), pivot_local.field_angle(), pivot_local.field_angle_2()));
						player_data->last_shoot_primitive = pivot_request;
						break;
					default:
						break;
				}
			}
			else {
				player.move_shoot(robot_local_dest, nav_request.params[3], nav_request.extra & 1);
			}
			break;
		case Drive::Primitive::CATCH:
			player.move_catch(Angle::of_radians(nav_request.params[0]), nav_request.params[1], nav_request.params[2]);
			break;
		case Drive::Primitive::PIVOT:
			LOG_DEBUG(Glib::ustring::compose("time for new pivot (pos %1, angle %2)", hl_request.field_point(), hl_request.field_angle()));
			pivot_local = pivot_in_local_coord(player, hl_request);
			player.move_pivot(pivot_local.field_point(), pivot_local.field_angle(), pivot_local.field_angle_2());
			break;
		case Drive::Primitive::SPIN:
			local_coord = move_in_local_coord(player, nav_request);
			LOG_DEBUG(Glib::ustring::compose("Time for new spin, point %1, angle speed %2", local_coord.field_point(), hl_request.field_angle()));
			player.move_spin(local_coord.field_point(), hl_request.field_angle());
			break;
		default:
			LOG_ERROR(Glib::ustring::compose("Unhandled primitive! (%1)", static_cast<int>(nav_request.prim)));
			break;
	}
*/
	// then there are movement types that has zoning logic, new primitive may be created even when highlevel request has not changed

	//if (hl_request.prim == Drive::Primitive::SHOOT && (hl_request.extra & 2) /* care angle */ && player_data->fancy_shoot_maneuver) {
		// evaluate whether we are facing the right way to kick on target
	/*
		Point target_position = Point::of_angle(hl_request.field_angle()) + hl_request.field_point();
		ShootActionType zone = shoot_movement_sequence(player, hl_request.field_point(), target_position, player_data->last_shoot_primitive);

		PrimitiveDescriptor shoot_request, pivot_local, local_coord;

		switch (zone) {
			case NO_ACTION_OR_PIVOT:
				// do nothing because player is close to a zone boundary
			case NO_ACTION_OR_MOVE:
				// do the last thing it did
				shoot_request = player_data->last_shoot_primitive;

				if (shoot_request.prim == Drive::Primitive::MOVE) {
					LOG_DEBUG(Glib::ustring::compose("new move (pos %1, angle %2)", hl_request.field_point(), hl_request.field_angle() ));
					local_coord = move_in_local_coord(player, shoot_request);
					player.move_move(local_coord.field_point(), local_coord.field_angle(), hl_request.params[3] * nav_request_len / hl_request_len);
					LOG_DEBUG(Glib::ustring::compose("local parameter for move, angle %1, point %2", local_coord.field_point(), local_coord.field_angle()));
				}
				else if (shoot_request.prim == Drive::Primitive::SHOOT) {
					if (!defense_area_violation) {
						LOG_DEBUG(Glib::ustring::compose("new shoot starting (pos %1, angle%2)", hl_request.field_point(), hl_request.field_angle() ));
						local_coord = move_in_local_coord(player, shoot_request);
						player.move_shoot(local_coord.field_point(), local_coord.field_angle(), hl_request.params[3], hl_request.extra & 1);
						LOG_DEBUG(Glib::ustring::compose("local params for shoot, angle %1, point %2", local_coord.field_point(), local_coord.field_angle()));
					} else {
						LOG_DEBUG(Glib::ustring::compose("defense violation dest %1", hl_request.field_point()));
						player.move_move(Point(0, 0));
					}
				} else if (shoot_request.prim == Drive::Primitive::PIVOT) {
					LOG_DEBUG(Glib::ustring::compose("new pivot starting(pos%1, angle%2)", hl_request.field_point(), hl_request.field_angle()));
					pivot_local = pivot_in_local_coord(player, shoot_request);
					player.move_pivot(pivot_local.field_point(), pivot_local.field_angle(), pivot_local.field_angle());
					LOG_DEBUG(Glib::ustring::compose("local parameter, angle %1, point %2", pivot_local.field_point(), pivot_local.field_angle_2()));
				}

				break;
			case SHOULD_SHOOT: // shooting
				shoot_request = hl_request;

				if (!defense_area_violation) {
					LOG_DEBUG(Glib::ustring::compose("new shoot starting(pos%1, angle%2)", hl_request.field_point(), hl_request.field_angle() ));

					local_coord = move_in_local_coord(player, shoot_request);
					player.move_shoot(local_coord.field_point(), local_coord.field_angle(), hl_request.params[3], hl_request.extra & 1);
					LOG_DEBUG(Glib::ustring::compose("local parameter for shoot, angle %1, point %2", local_coord.field_point(), local_coord.field_angle()));
					player_data->last_shoot_primitive = shoot_request;
				} else {
					LOG_DEBUG(Glib::ustring::compose("defense violation, dest %1", hl_request.field_point()));
					player.move_move(Point(0, 0));
				}
				break;
			case SHOULD_MOVE: // moves a little closer to pivot center
				shoot_request = hl_request;
				shoot_request.prim = Drive::Primitive::MOVE;
				LOG_DEBUG(Glib::ustring::compose("new move starting first time (pos%1, angle%2)", hl_request.field_point(), hl_request.field_angle()));
				local_coord = move_in_local_coord(player, shoot_request);
				player.move_move(local_coord.field_point(), local_coord.field_angle());
				LOG_DEBUG(Glib::ustring::compose("local parameter for move, angle %1, point %2", local_coord.field_point(), local_coord.field_angle()));
				player_data->last_shoot_primitive = shoot_request;
				break;
			case SHOULD_PIVOT: // pivot around ball
				shoot_request.prim = Drive::Primitive::PIVOT;
				shoot_request.params[0] = hl_request.field_point().x;
				shoot_request.params[1] = hl_request.field_point().y;
				shoot_request.params[2] = hl_request.field_angle().to_radians();
				LOG_DEBUG(Glib::ustring::compose("new pivot starting (pos%1, angle%2)", hl_request.field_point(), hl_request.field_angle()));
				pivot_local = pivot_in_local_coord(player, shoot_request);
				player.move_pivot(pivot_local.field_point(), pivot_local.field_angle(), pivot_local.field_angle_2());
				player_data->last_shoot_primitive = shoot_request;
				LOG_DEBUG(Glib::ustring::compose("local parameter, angle %1, point %2", pivot_local.field_point(), pivot_local.field_angle()));
				break;
			default:
				break;
		}
	}

*/
	// TODO test if primitive is done
}

void RRTNavigator::tick() {
	for (Player player : world.friendly_team()) {
		plan(player);
	}
}

NAVIGATOR_REGISTER(RRTNavigator)
