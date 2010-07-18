#include "ai/navigator/testnavigator.h"
#include <iostream>
#include <cstdlib>
TestNavigator::TestNavigator(Player::ptr player, World::ptr world) : 
  the_player(player), the_world(world), destInitialized(false), outOfBoundsMargin(the_world->field().width() / 20.0),
  maxLookahead(1.0), aggression_factor(2)
{

}

void TestNavigator::tick() {
  const Field &the_field(the_world->field());
  const Ball::ptr the_ball(the_world->ball());

  //tell it which way to go
  if(destInitialized)
    {
      
      Point nowDest;

      // if we have the ball, adjust our destination to ensure that we
      // don't take the ball out of bounds, otherwise, head to our
      // assigned destination
#warning has ball
      if (the_player->sense_ball())
	{
     
	  nowDest = clip_point(currDest,
			       Point(-the_field.length()/2 + outOfBoundsMargin,
				     -the_field.width()/2 + outOfBoundsMargin),
			       Point(the_field.length()/2 - outOfBoundsMargin,
				     the_field.width()/2 - outOfBoundsMargin));
	}
      else
	{
	  nowDest = currDest;
	}

      Point direction = nowDest - the_player->position();

      if (direction.len() < 0.01)
	{
	  return;
	}

      double dirlen = direction.len();
      direction = direction / direction.len();

      Point leftdirection = direction;
      Point rightdirection = direction;

      double rotationangle = 0.0;

      bool undiverted = true;
      bool stop = false;
      bool chooseleft;
      while (true)
	{
	  //std::cout << "path changed" <<std::endl;
	  
	  //it shouldn't take that many checks to get a good direction
	  leftdirection = direction.rotate(rotationangle);
	  rightdirection = direction.rotate(-rotationangle);

	  if (check_vector(the_player->position(), nowDest, leftdirection.rotate(2.5 * M_PI / 180.0)))
	    {
	      if (check_vector(the_player->position(), nowDest, leftdirection.rotate(-2.5 * M_PI / 180.0)))
		{
		  chooseleft = true;
		  break;
		}
	    }
	  else if (check_vector(the_player->position(), nowDest, rightdirection.rotate(-2.5 * M_PI / 180.0)))
	    {
	      if (check_vector(the_player->position(), nowDest, rightdirection.rotate(2.5 * M_PI / 180.0)))
		{
		  chooseleft = false;
		  break;
		}
	    }
	  
	  // if we can't find a path within 90 degrees
	  // go straight towards our destination
	  if (rotationangle > 100.0 * M_PI / 180.0)
	    {
	      leftdirection = rightdirection = direction;
	      stop = true;
	      break;
	    }
	  rotationangle += 1.0 * M_PI / 180.0;//rotate by 1 degree each
					    //time
	}
      undiverted = rotationangle < 1e-5;
      if(stop)
	{
	  Point balldest = the_ball->position() - the_player->position();
	  the_player->move(the_player->position(), atan2(balldest.y, balldest.x));
	}
      else
	{
	  Point selected_direction;
	  if (chooseleft)
	    {
	      // select the left vector
	      selected_direction = leftdirection;     
	    }
	  else
	    {
	      // select the right vector
	      selected_direction = rightdirection;
	    }
	  
	  if (undiverted)
	    {
	      Point balldest = the_ball->position() - the_player->position();
	      the_player->move(nowDest, atan2(balldest.y, balldest.x));
	    }
	  else
	    {
	      // maximum warp
	      Point balldest = the_ball->position() - the_player->position();
	      the_player->move(the_player->position() + selected_direction*std::min(dirlen,1.0), atan2(balldest.y, balldest.x));
	    }
	}
    }
}

void TestNavigator::set_point(const Point &destination) {
  //set new destinatin point
  destInitialized = true;
  /*currDest = clip_point(destination,
			Point(-the_field->length()/2,-the_field->width()/2),
			Point(the_field->length()/2,the_field->width()/2));*/
  currDest = destination;
}

Point TestNavigator::clip_point(Point p, Point bound1, Point bound2)
{
  Point rv;

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

bool TestNavigator::check_vector(Point start, Point dest, Point direction)
{

  Point startdest = dest - start;

  double lookahead = std::min(startdest.len(), maxLookahead);

  const Team * const teams[2] = { &the_world->friendly, &the_world->enemy };
  for (unsigned int i = 0; i < 2; ++i)
    {
      for (unsigned int j = 0; j < teams[i]->size(); ++j)
	{
	  const Robot::ptr rob(teams[i]->get_robot(j));
	  
	  if(rob != this->the_player)
	    {
	      Point rp = rob->position() - start;
	      //rp/= rp.len();
	      double len = rp.dot(direction);

	      // ignore robots behind us
	      if (len > 0)
		{
		  double d = sqrt(rp.dot(rp) - len*len);
	      
		  if (len < lookahead && d < 2*aggression_factor*Robot::MAX_RADIUS)
		    {
		      //std::cout << "Checked FALSE" << std::endl;
		      return false;
		    }
		}
	    }
	}
    }
  //std::cout << "Checked TRUE" << std::endl;
  return true;

}
