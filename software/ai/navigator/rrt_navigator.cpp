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

namespace AI {
	namespace Nav {
		namespace RRT {
			// fraction of the maximum speed that the robot will try to dribble at
			const double DRIBBLE_SPEED = 0.4;

			IntParam jon_hysteris_hack(u8"Jon Hysteris Hack", u8"AI/Navigator/RRT", 2, 1, 10);

			DoubleParam move_update_time(u8"Time before we update the movement primitives", u8"AI/MovementPrimitives", 0.1, 0.01, 1.0);
			DoubleParam move_update_distance(u8"Distance before we update the movement primitives", u8"AI/Movement/Primitives", 10, 0, 1000.0);
			DoubleParam primitive_update_count(u8"Tick before update primitive", u8"AI/Movement/Primitives", 15, 0, 100.0);
			DoubleParam shoot_update_count(u8"Tick before update shoot primitive", u8"AI/Movement/Primitives", 15, 0, 100.0);
			DoubleParam pivot_update_count(u8"Tick before update pivot primitive", u8"AI/Movement/Primitives", 15, 0, 100.0);
			DoubleParam move_update_count(u8"Tick before update move primitive", u8"AI/Movement/Primitives", 15, 0, 100.0);
			DoubleParam default_desired_rpm(u8"The default desired rpm for dribbling", u8"AI/Movement/Primitives", 15, 0, 100.0);



			class RRTNavigator final : public Navigator {
				public:
					explicit RRTNavigator(World world);
					void tick() override;
					void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override;
					NavigatorFactory &factory() const override;
					enum shoot_action_type{
						NO_ACTION_OR_PIVOT=0,
						SHOULD_SHOOT,
						SHOULD_MOVE,
						SHOULD_PIVOT,
						NO_ACTION_OR_MOVE,
						SHOOT_FAILED
					};

				private:
					bool primitive_diff(AI::PrimitiveInfo cur, AI::PrimitiveInfo request);
					shoot_action_type shoot_movement_zoning(Player player, Point ball_pos, Point target_pos);
					shoot_action_type shoot_movement_sequence(Player player, Point ball_pos, Point target_pos, AI::PrimitiveInfo last_primitive);
					AI::PrimitiveInfo move_in_local_coord(Player player, AI::PrimitiveInfo global);
					AI::PrimitiveInfo pivot_in_local_coord(Player player, AI::PrimitiveInfo shoot_pivot_request);
					RRTPlanner planner;
					bool is_ccw;
			};
		}
	}
}

using AI::Nav::RRT::RRTNavigator;

RRTNavigator::RRTNavigator(AI::Nav::W::World world) : Navigator(world), planner(world) {
}

bool RRTNavigator::primitive_diff(AI::PrimitiveInfo cur, AI::PrimitiveInfo request){
	if(cur.type != request.type){
		return true;
	} else {
	
		Point point_diff = cur.field_point-request.field_point;
		Angle angle_diff = cur.field_angle.angle_diff(request.field_angle);
		bool angle_care_diff = (cur.care_angle!=request.care_angle);
		bool chip_diff = (cur.field_bool != request.field_bool);
		double power_diff = cur.field_double - request.field_double;

		// handle dest
		if( point_diff.len() > 0.05 || angle_care_diff ){
			return true;
		}

		// handle angle if care
		if( request.care_angle && (angle_diff.abs() > Angle::of_degrees(5)) ){	
			return true;
		}

		// handle chip/kick
		if( request.type == Drive::Primitive::SHOOT && (chip_diff || std::abs(power_diff) > 1.0) ){
			return true;
		}
	}
	return false;
}

RRTNavigator::shoot_action_type RRTNavigator::shoot_movement_sequence(Player player, Point ball_pos, Point target_pos, AI::PrimitiveInfo last_primitive) {
	shoot_action_type zone = shoot_movement_zoning(player, ball_pos, target_pos);
	if(last_primitive.type==Drive::Primitive::SHOOT){
		if (zone != SHOOT_FAILED ){
			return SHOULD_SHOOT;
		} else {
			return SHOULD_PIVOT;
		}
	} else if(last_primitive.type==Drive::Primitive::PIVOT){
		if( zone != SHOULD_SHOOT ){
			return SHOULD_PIVOT;
		} else {
			return SHOULD_SHOOT;
		}
	}  else {
		return zone;
	}
}

RRTNavigator::shoot_action_type RRTNavigator::shoot_movement_zoning (Player player,
	Point ball_pos, Point target_pos)
{
	Point ball2target = target_pos - ball_pos;
	Point ball2player = player.position() - ball_pos;
	Angle swing_diff = (ball2target.orientation() - ball2player.orientation() +
		Angle::half()).angle_mod();

	if (swing_diff.abs() <= Angle::of_degrees(5) ) {
		// within angle tolerance of pivot
		return SHOULD_SHOOT;
	} else if (swing_diff.abs() <= Angle::of_degrees(90) && ball2player.len()
		<= Robot::MAX_RADIUS )
	{
		if (std::fabs(ball2player.dot(ball2target)) <= 0.3 * Robot::MAX_RADIUS) {	
			// within distance tolerance to shoot ball, but geometry is really off
			return SHOOT_FAILED;
		} else {
			// within distance tolerance to shoot ball
			return SHOULD_SHOOT;
		}
	} /*else if (ball2player.len() <= Robot::MAX_RADIUS*2 ) {
		
		// boundary between pivot and shoot
		return NO_ACTION_OR_PIVOT;
	}*/ else if (swing_diff.abs() > Angle::of_degrees(5) &&
			swing_diff.abs() <= Angle::of_degrees(50) )
	{
		// boundary between pivot, shoot and move
		return NO_ACTION_OR_PIVOT;
	} else if(ball2player.len() < 1.0) {
		// close to ball but angle is too off
		return SHOULD_PIVOT; 
	} else if (ball2player.len() < 1.4) {
		// Boundary between pivot and move
		return NO_ACTION_OR_MOVE;
	} else {
		// Get further away so we can pivot safely.
		return SHOULD_MOVE;
	}
}


AI::PrimitiveInfo RRTNavigator::pivot_in_local_coord(Player player, AI::PrimitiveInfo global) {
	Point player2centre = global.field_point - player.position();
	Angle pivot_swing = (global.field_angle - player2centre.orientation() );
	Point pivot_dest_local = player2centre.rotate(-player.orientation());
	Angle rotate_angle_local = global.field_angle - player.orientation();

	AI::PrimitiveInfo local;
	local.type = global.type;
	local.field_point = pivot_dest_local;
	local.field_angle = pivot_swing;
	local.field_angle2 = rotate_angle_local;
	
	return local;
}

AI::PrimitiveInfo RRTNavigator::move_in_local_coord(Player player, AI::PrimitiveInfo global) {
	Point pos_diff = global.field_point-player.position();
	Point robot_local_dest = pos_diff.rotate(-player.orientation());
	Angle robot_local_angle = (global.field_angle - player.orientation());
	
	AI::PrimitiveInfo local;
	local.type = global.type;
	local.field_point = robot_local_dest;
	local.field_angle = robot_local_angle;
	
	return local;
}
void RRTNavigator::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
	ctx->set_source_rgb(1.0, 1.0, 1.0);
	for (const Player player : world.friendly_team()) {
		ctx->begin_new_path();
		ctx->set_line_width(1);
		ctx->move_to(world.ball().position().x, world.ball().position().y);
		ctx->line_to(player.destination().first.x, player.destination().first.y);
	}
}

void RRTNavigator::tick() {
	for (Player player : world.friendly_team()) {
		if (!std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)])) {
			player.object_store()[typeid(*this)] = std::make_shared<PlayerData>();
		}

		PlayerData::Ptr player_data = std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)]);
		player_data->added_flags = 0;

		AI::PrimitiveInfo hl_request = player.primitive_info();
		AI::PrimitiveInfo last_primitive = player_data->last_primitive;
		AI::PrimitiveInfo last_shoot_primitive = player_data->last_shoot_primitive;

		player_data->counter_since_last_primitive++;

		player.display_path({});

		// just a hack for now, defense logic should be implemented somewhere else
		// positive x is enemy goal
		double x_limit = world.field().enemy_goal().x-world.field().defense_area_radius()/2;
		double y_limit = (world.field().defense_area_stretch()+world.field().defense_area_radius()*2)/2;
		bool defense_area_violation = false;
		if( hl_request.field_point.x > x_limit && std::fabs(hl_request.field_point.y) < y_limit && player.flags()&AI::Flags::FLAG_AVOID_ENEMY_DEFENSE){
			defense_area_violation = true;
		}
		// convert to robot-relative coordinates
		//std::cout << "move type is " << hl_request.type << std::endl;
		//std::cout << "move type is " << hl_request.type << std::endl;

		// starting a primitive
		if( hl_request.type != last_primitive.type ||  player_data->counter_since_last_primitive >= primitive_update_count || primitive_diff(last_primitive, hl_request) ){
			LOG_DEBUG(Glib::ustring::compose("Time for new primitive (type=%1, hlr.fp=%2, hlr.fa=%3)", static_cast<unsigned int>(hl_request.type), hl_request.field_point, hl_request.field_angle));
			#warning they should be conditional upon different primitive type instead

			// plan
			AI::PrimitiveInfo nav_request = hl_request;
			switch (hl_request.type) {
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
					player.display_path({hl_request.field_point});
					if (!valid_path(player.position(), hl_request.field_point, world, player)) {
#warning Do we need flags here, e.g. to let the goalie into the defense area?
						const std::vector<Point> &plan = planner.plan(player, hl_request.field_point, 0);
						if (!plan.empty()) {
							nav_request.field_point = plan[0];
							player.display_path(plan);
						}
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


		//	nav_request = hl_request;a
			// execute the plan
			// some calculation that are generally useful
			Point pos_diff = nav_request.field_point-player.position();
			Point robot_local_dest = pos_diff.rotate(-player.orientation());
			//std::cout << "[nav] high level request move type is " <<(int) hl_request.type << std::endl;
			//std::cout << "[nav] high level request point is (" << hl_request.field_point.x  << ","<< hl_request.field_point.y <<")" << std::endl;
			//std::cout << "[nav] high level request angle is " << hl_request.field_angle.to_degrees() << std::endl;
			//std::cout << "[nav] robot local coord is " << robot_local_dest.x << ", " << robot_local_dest.y << std::endl;
			//std::cout << "[nav] robot local rotation is " << robot_local_angle.to_degrees() << std::endl;

			// some shoot related stuff

			shoot_action_type zone;

			AI::PrimitiveInfo pivot_request;
			AI::PrimitiveInfo pivot_local;
			AI::PrimitiveInfo shoot_request;
			AI::PrimitiveInfo local_coord;
			Point target_position = Point::of_angle(hl_request.field_angle)+hl_request.field_point;

			if( hl_request.type != Drive::Primitive::SHOOT ){
				player_data->last_shoot_primitive.type = Drive::Primitive::MOVE;
			}

			switch (nav_request.type) {
				case Drive::Primitive::MOVE:	
					if( nav_request.care_angle ){
						
						local_coord = move_in_local_coord( player, nav_request );
						LOG_DEBUG(Glib::ustring::compose("Time for new move, point %1, angl %2",  local_coord.field_point, local_coord.field_angle));
						player.move_move(local_coord.field_point, local_coord.field_angle);
					} else {
						nav_request.field_angle = Angle(); // fill in angle so the function doesn't crash
						local_coord = move_in_local_coord( player, nav_request );
						LOG_DEBUG(Glib::ustring::compose("Time for new move, point %1",  local_coord.field_point));
						player.move_move(local_coord.field_point);
					}
					break;
				case Drive::Primitive::DRIBBLE:	

					local_coord = move_in_local_coord( player, nav_request );
					LOG_DEBUG(Glib::ustring::compose("Time for new dribble point %1, angl %2",  local_coord.field_point, local_coord.field_angle));
					player.move_dribble(local_coord.field_point, local_coord.field_angle, default_desired_rpm, false);
					break;
				case Drive::Primitive::SHOOT:	
					if( nav_request.care_angle ){
						// evaluate if we need to do a maneuver
						player_data->fancy_shoot_maneuver = true;
						// evaluate whether we are facing the right way to kick on target
						zone  = shoot_movement_sequence(player, nav_request.field_point, target_position, player_data->last_shoot_primitive);
						switch( zone ){
						//case 0: // do nothing because player is close to a zone boundary
						//	break;
						case SHOULD_SHOOT: // shooting

							LOG_DEBUG(Glib::ustring::compose("new shoot starting first time (pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));
	
							local_coord = move_in_local_coord( player, hl_request );
							shoot_request = hl_request;
							player.move_shoot(local_coord.field_point, local_coord.field_angle, hl_request.field_double, hl_request.field_bool);
							LOG_DEBUG(Glib::ustring::compose("local parameter for shoot, angle %1, point %2", local_coord.field_point, local_coord.field_angle));
							player_data->last_shoot_primitive = shoot_request;
							player_data->counter_since_last_shoot_primitive = 0;
							
							break;
						case SHOULD_MOVE: // moves a little closer to pivot center
						case NO_ACTION_OR_MOVE:
							LOG_DEBUG(Glib::ustring::compose("new move starting first time (pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));
							local_coord = move_in_local_coord( player, hl_request );
							player.move_move(local_coord.field_point, local_coord.field_angle);
							LOG_DEBUG(Glib::ustring::compose("local parameter for move, angle %1, point %2", local_coord.field_point, local_coord.field_angle));
							shoot_request = hl_request;
							shoot_request.type = Drive::Primitive::MOVE;
							player_data->last_shoot_primitive = shoot_request;
							player_data->counter_since_last_shoot_primitive = 0;
							break;
						case NO_ACTION_OR_PIVOT: // when in doubt, do pivot
						case SHOULD_PIVOT: // pivot around ball
							LOG_DEBUG(Glib::ustring::compose("new pivot starting first time (pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));
							pivot_request = hl_request;
							pivot_request.type = Drive::Primitive::PIVOT;
							pivot_local = pivot_in_local_coord( player, pivot_request );
							player.move_pivot(pivot_local.field_point, pivot_local.field_angle, pivot_local.field_angle2);
							LOG_DEBUG(Glib::ustring::compose("local parameter for pivot, center %1, swing %2, rotation %3", pivot_local.field_point, pivot_local.field_angle, pivot_local.field_angle2));
							player_data->last_shoot_primitive = pivot_request;
							player_data->counter_since_last_shoot_primitive = 0;
							break;
						default:
							break;
						}
					} else { 
						player.move_shoot(robot_local_dest, nav_request.field_double, nav_request.field_bool);
					}
					break;
				case Drive::Primitive::CATCH:	
					//player.move_catch(nav_request.field_point-player.position(), nav_request.field_angle-player.orientation(), 0.0, false);
					break;
				case Drive::Primitive::PIVOT:	
					LOG_DEBUG(Glib::ustring::compose("time for new pivot (pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));
					pivot_local = pivot_in_local_coord( player, hl_request );
					player.move_pivot(pivot_local.field_point, pivot_local.field_angle, pivot_local.field_angle2);
					break;
				case Drive::Primitive::SPIN:	
					local_coord = move_in_local_coord( player, nav_request );
					LOG_DEBUG(Glib::ustring::compose("Time for new spin, point %1, angle speed %2",  local_coord.field_point, hl_request.field_angle));
					player.move_move(local_coord.field_point, hl_request.field_angle);
					break;
				default:
					//assert(false); //die
					break;
			}
			player_data->counter_since_last_primitive = 0;
			player_data->last_primitive = hl_request;
		}

		// then there are movement types that has zoning logic, new primitive may be created even when highlevel request has not changed
		if( hl_request.type == Drive::Primitive::SHOOT &&  hl_request.care_angle && player_data->fancy_shoot_maneuver ){
			player_data->counter_since_last_primitive = 0;// reset this ever tick to prevent shoot's sub actions to be called all the time
			// evaluate whether we are facing the right way to kick on target

			Point target_position = Point::of_angle(hl_request.field_angle)+hl_request.field_point;
			shoot_action_type zone  = shoot_movement_sequence(player, hl_request.field_point, target_position, player_data->last_shoot_primitive);
			AI::PrimitiveInfo shoot_request;
			AI::PrimitiveInfo pivot_local;
			AI::PrimitiveInfo local_coord;
			
			player_data->counter_since_last_shoot_primitive++;

			switch( zone ){
			case NO_ACTION_OR_PIVOT: // do nothing because player is close to a zone boundary
			case NO_ACTION_OR_MOVE:
				// do the last thing it did
				shoot_request = player_data->last_shoot_primitive;
				
				if(  player_data->counter_since_last_shoot_primitive >= shoot_update_count || primitive_diff(player_data->last_shoot_primitive, shoot_request) ){ // need to check counter
					if( shoot_request.type == Drive::Primitive::MOVE ) {
						LOG_DEBUG(Glib::ustring::compose("new move atarting(pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));

						local_coord = move_in_local_coord( player, shoot_request );
						player.move_move(local_coord.field_point, local_coord.field_angle);
						LOG_DEBUG(Glib::ustring::compose("local parameter for move, angle %1, point %2", local_coord.field_point, local_coord.field_angle));
					} else if ( shoot_request.type == Drive::Primitive::SHOOT ) {
						if( !defense_area_violation ){
							LOG_DEBUG(Glib::ustring::compose("new shoot atarting(pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));

							local_coord = move_in_local_coord( player, shoot_request );
							player.move_shoot(local_coord.field_point, local_coord.field_angle, hl_request.field_double, hl_request.field_bool);
							LOG_DEBUG(Glib::ustring::compose("local parameter for shoot, angle %1, point %2", local_coord.field_point, local_coord.field_angle));
						} else {
							LOG_DEBUG(Glib::ustring::compose("defense violation dest %1", hl_request.field_point));
							player.move_move(Point(0,0));
							
						}
					} else if ( shoot_request.type == Drive::Primitive::PIVOT ) {
						LOG_DEBUG(Glib::ustring::compose("new pivot atarting(pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));

						pivot_local = pivot_in_local_coord( player, shoot_request );
						player.move_pivot(pivot_local.field_point, pivot_local.field_angle, pivot_local.field_angle2);
						LOG_DEBUG(Glib::ustring::compose("local parameter, angle %1, point %2", pivot_local.field_point, pivot_local.field_angle));
					}

				//player_data->last_shoot_primitive = shoot_request;
					player_data->counter_since_last_shoot_primitive = 0;
				}
				
				break;
			case SHOULD_SHOOT: // shooting
				shoot_request = hl_request;
				if(  player_data->counter_since_last_shoot_primitive >= shoot_update_count || primitive_diff(player_data->last_shoot_primitive, shoot_request) ){ // need to check counter
					if( !defense_area_violation ){
						LOG_DEBUG(Glib::ustring::compose("new shoot starting(pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));

						local_coord = move_in_local_coord( player, shoot_request );
						player.move_shoot(local_coord.field_point, local_coord.field_angle, hl_request.field_double, hl_request.field_bool);
						LOG_DEBUG(Glib::ustring::compose("local parameter for shoot, angle %1, point %2", local_coord.field_point, local_coord.field_angle));
						player_data->last_shoot_primitive = shoot_request;
						player_data->counter_since_last_shoot_primitive = 0;
					}  else {
						LOG_DEBUG(Glib::ustring::compose("defense violation, dest %1", hl_request.field_point));
						player.move_move(Point(0,0));
							
					}
				}
				break;
			case SHOULD_MOVE: // moves a little closer to pivot center
				shoot_request = hl_request;
				shoot_request.type = Drive::Primitive::MOVE;
				if(   player_data->counter_since_last_shoot_primitive >= move_update_count || primitive_diff(player_data->last_shoot_primitive, shoot_request) ){ // need to check counter
					LOG_DEBUG(Glib::ustring::compose("new move starting first time (pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));
					local_coord = move_in_local_coord( player, shoot_request );
					player.move_move(local_coord.field_point, local_coord.field_angle);
					LOG_DEBUG(Glib::ustring::compose("local parameter for move, angle %1, point %2", local_coord.field_point, local_coord.field_angle));
					player_data->last_shoot_primitive = shoot_request;
					player_data->counter_since_last_shoot_primitive = 0;
				}
				break;
			case SHOULD_PIVOT: // pivot around ball
				shoot_request.type = Drive::Primitive::PIVOT;
				shoot_request.field_point = hl_request.field_point;
				shoot_request.field_angle = hl_request.field_angle;
				if(    player_data->counter_since_last_shoot_primitive >= pivot_update_count || primitive_diff(player_data->last_shoot_primitive, shoot_request) ){ // need to check counter			
					LOG_DEBUG(Glib::ustring::compose("new pivot starting (pos%1, angle%2)", hl_request.field_point, hl_request.field_angle ));
					pivot_local = pivot_in_local_coord( player, shoot_request );
					player.move_pivot(pivot_local.field_point, pivot_local.field_angle, pivot_local.field_angle2);
					player_data->last_shoot_primitive = shoot_request;
					player_data->counter_since_last_shoot_primitive = 0;
					LOG_DEBUG(Glib::ustring::compose("local parameter, angle %1, point %2", pivot_local.field_point, pivot_local.field_angle));
				}
				break;
			default:
				break;
			}
			
		} 

	}
}

NAVIGATOR_REGISTER(RRTNavigator)

