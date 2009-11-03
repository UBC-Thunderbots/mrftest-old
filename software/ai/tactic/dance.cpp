#include "ai/tactic/dance.h"
#include "ai/navigator/testnavigator.h"
#include "geom/angle.h"

dance::dance(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) , the_navigator(new testnavigator(player,field)) {
    tick = 0;
    the_player = player;
}

// Rotate left right left right left right.
void dance::update()
{
    #define STEP 15 // how many steps it takes for robot to change direction
    
    // Position stays the same, but orientation should turn left and right...
	the_player->move(the_player->position(),
                     the_player->orientation()+PI*(((tick/STEP)%2)?1:-1));
	the_navigator->update();

    // keep track of 'clock' tick
    tick++;
}
