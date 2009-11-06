#include "ai/navigator/testnavigator.h"
#include "ai/tactic/dance.h"

dance::dance(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player) , the_navigator(new testnavigator(player, field, ball, team)) {
    ticks = 0;
    the_player = player;
}

// Rotate left right left right left right.
void dance::tick()
{
    #define STEP 15 // how many steps it takes for robot to change direction
    
    // Position stays the same, but orientation should turn left and right...
	the_player->move(the_player->position(),
                     the_player->orientation()+PI*(((ticks/STEP)%2)?1:-1));
	the_navigator->tick();

    // keep track of 'clock' tick
    ticks++;
}
