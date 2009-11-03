#include "ai/navigator/testnavigator.h"
#include "world/field.h"

testnavigator::testnavigator(player::ptr player, field::ptr field) : 
  navigator(player, field), destInitialized(false), outOfBoundsMargin(1.0)
{

}

void testnavigator::update() {
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
			       point(-the_field->width()/2 + outOfBoundsMargin,
				     -the_field->length()/2 + outOfBoundsMargin),
			       point(the_field->width()/2 - outOfBoundsMargin,
				     the_field->length()/2 - outOfBoundsMargin));
	}
      else
	{
	  nowDest = currDest;
	}
    }
}

void testnavigator::set_point(const point &destination) {
  //set new destinatin point
  destInitialized = true;

  currDest = clip_point(destination,
			point(-the_field->width()/2,
			      -the_field->length()/2),
			point(the_field->width()/2,
			      the_field->length()/2));
}

point testnavigator::clip_point(point p, point bound1, point bound2)
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

