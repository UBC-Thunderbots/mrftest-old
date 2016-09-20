#pragma once
#include "ai/hl/stp/world.h"
#include "ai/backend/primitives/all.h"
#include <map>

namespace Primitives = AI::BE::Primitives;

namespace AI {
	namespace HL {
		namespace STP {
			namespace LegacyAction {
				extern std::map<AI::Common::Player, Primitives::Primitive::Ptr> primitives;
				inline void clear_legacy_prim(AI::Common::Player p) {
					primitives[p].reset();
				}
			}
		}
	}
}

using AI::HL::STP::LegacyAction::primitives;

// macro overload resolver
#define GET_OVERLOAD(A,B,C,D,E,F,G,H,NAME,...) NAME

#define mp_stop(...) GET_OVERLOAD(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, mp_stop1)(__VA_ARGS__)
#define mp_stop1(BOOL) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Stop(player, BOOL));

#define mp_move(...) GET_OVERLOAD(__VA_ARGS__, 8, 7, 6, 5, 4, 3, mp_move2, mp_move1)(__VA_ARGS__)
#define mp_move1(POINT) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Move(player, POINT));
#define mp_move2(POINT, ANGLE) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Move(player, POINT, ANGLE));

#define mp_spin(...) GET_OVERLOAD(__VA_ARGS__, 8, 7, 6, 5, 4, 3, mp_spin2, 1)(__VA_ARGS__)
#define mp_spin2(POINT, ANGLE) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Spin(player, POINT, ANGLE));

#define mp_catch(...) GET_OVERLOAD(__VA_ARGS__, 8, 7, 6, 5, 4, mp_catch3, 2, 1)(__VA_ARGS__)
#define mp_catch3(ANGLE, DOUBLE1, DOUBLE2) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Catch(player, ANGLE, DOUBLE1, DOUBLE2));

#define mp_dribble(...) GET_OVERLOAD(__VA_ARGS__, 8, 7, 6, 5, 4, mp_dribble3, mp_dribble2, 1)(__VA_ARGS__)
#define mp_dribble3(POINT, ANGLE, BOOL) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Dribble(player, POINT, ANGLE, BOOL));
#define mp_dribble2(POINT, ANGLE) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Dribble(player, POINT, ANGLE, false));

#define mp_shoot(...) GET_OVERLOAD(__VA_ARGS__, 8, 7, 6, 5, mp_shoot4, mp_shoot3, 2, 1)(__VA_ARGS__)
#define mp_shoot3(POINT, DOUBLE, BOOL) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Shoot(player, POINT, DOUBLE, BOOL));
#define mp_shoot4(POINT, ANGLE, DOUBLE, BOOL) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Shoot(player, POINT, ANGLE, DOUBLE, BOOL));

#define mp_pivot(...) GET_OVERLOAD(__VA_ARGS__, 8, 7, 6, 5, 4, mp_pivot3, mp_pivot2, 1)(__VA_ARGS__)
#define mp_pivot3(POINT, ANGLE1, ANGLE2) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Pivot(player, POINT, ANGLE1, ANGLE2));
#define mp_pivot2(POINT, ANGLE1) flags(); \
	primitives[player].reset(); \
	primitives[player] = Primitives::Primitive::Ptr(new Primitives::Pivot(player, POINT, ANGLE1, Angle::zero()));
