#include "ai/navigator.h"
#include "world/field.h"

navigator::navigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team) : the_player(player), the_field(field), the_ball(ball), the_team(team) {
}

