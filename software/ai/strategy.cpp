#include "ai/strategy.h"

strategy::strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : the_ball(ball), the_field(field), the_team(team), pt_source(pt_src) {
	team->signal_player_added().connect(sigc::mem_fun(*this, &strategy::robot_added));
	team->signal_player_removed().connect(sigc::mem_fun(*this, &strategy::robot_removed));
	pt_source.signal_playtype_changed().connect(sigc::mem_fun(*this, &strategy::set_playtype));
}

