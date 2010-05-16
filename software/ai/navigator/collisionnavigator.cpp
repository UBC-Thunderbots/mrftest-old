#include "ai/navigator/collisionnavigator.h"
#include <iostream>
collisionnavigator::collisionnavigator(player::ptr player, world::ptr world) : 
  the_player(player), the_world(world), destInitialized(false), outOfBoundsMargin(world->field().width() / 20.0),
  maxLookahead(1.0)
{

}

void collisionnavigator::tick() {
  const field &the_field(the_world->field());
  const ball::ptr the_ball(the_world->ball());

  //tell it which way to go
  if(destInitialized)
    {
      
      point nowDest;

      // if we have the ball, adjust our destination to ensure that we
      // don't take the ball out of bounds, otherwise, head to our
      // assigned destination
      if (the_player->has_ball())
	{
     
	  nowDest = clip_point(currDest,
			       point(-the_field.length()/2 + outOfBoundsMargin,
				     -the_field.width()/2 + outOfBoundsMargin),
			       point(the_field.length()/2 - outOfBoundsMargin,
				     the_field.width()/2 - outOfBoundsMargin));
	}
      else
	{
	  nowDest = currDest;
	}

      point direction = nowDest - the_player->position();

      if (direction.len() < 0.01)
	{
	  return;
	}

      point balldest = the_ball->position() - the_player->position();
      the_player->move(nowDest, atan2(balldest.y, balldest.x));

    }
}

void collisionnavigator::set_point(const point &destination) {
  const field &the_field(the_world->field());

  //set new destinatin point
  destInitialized = true;

  currDest = clip_point(destination,
			point(-the_field.length()/2,-the_field.width()/2),
			point(the_field.length()/2,the_field.width()/2));
}

point collisionnavigator::clip_point(point p, point bound1, point bound2)
{
  point rv;

  double minx,maxx,miny,maxy;

  if (bound1.x < bound2.x)
    {
      minx = bound1.x;
      maxx = bound2.x;
    }
  else
    {
      minx = bound2.x;
      maxx = bound1.x;
    }

  if (bound1.y < bound2.y)
    {
      miny = bound1.y;
      maxy = bound2.y;
    }
  else
    {
      miny = bound2.y;
      maxy = bound1.y;
    }

  if (p.x < minx)
    {
      rv.x = minx;
    }
  else if (p.x > maxx)
    {
      rv.x = maxx;      
    }
  else
    {
      rv.x = p.x;
    }

  if (p.y < miny)
    {
      rv.y = miny;
    }
  else if (p.y > maxy)
    {
      rv.y = maxy;
    }
  else
    {
      rv.y = p.y;
    }

  return rv;
}
