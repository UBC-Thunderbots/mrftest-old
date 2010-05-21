#include "ai/tactic/dance.h"

dance::dance(player::ptr player) : the_player(player) {
    ticks = 0;
}

// Rotate left right left right left right.
void dance::tick()
{
    #define STEP 15 // how many steps it takes for robot to change direction
    
    // Position stays the same, but orientation should turn left and right...
	the_player->move(the_player->position(),
                     the_player->orientation()+M_PI*(((ticks/STEP)%2)?1:-1));

    // keep track of 'clock' tick
    ticks++;
}
