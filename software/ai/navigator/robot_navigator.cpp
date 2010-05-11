#include "ai/navigator/robot_navigator.h"
#include <iostream>
#include <cstdlib>

robot_navigator::robot_navigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team) : 
  navigator(player, field, ball, team), destInitialized(false), outOfBoundsMargin(field->width() / 20.0),
  maxLookahead(1.0), aggression_factor(2)
{

}

void robot_navigator::tick() {
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
			       point(-the_field->length()/2 + outOfBoundsMargin,
				     -the_field->width()/2 + outOfBoundsMargin),
			       point(the_field->length()/2 - outOfBoundsMargin,
				     the_field->width()/2 - outOfBoundsMargin));
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

      double dirlen = direction.len();
      direction = direction / direction.len();

      point leftdirection = direction;
      point rightdirection = direction;

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

	  if (check_vector(the_player->position(), nowDest, leftdirection.rotate(2.5 * PI / 180.0)))
	    {
	      if (check_vector(the_player->position(), nowDest, leftdirection.rotate(-2.5 * PI / 180.0)))
		{
		  chooseleft = true;
		  break;
		}
	    }
	  else if (check_vector(the_player->position(), nowDest, rightdirection.rotate(-2.5 * PI / 180.0)))
	    {
	      if (check_vector(the_player->position(), nowDest, rightdirection.rotate(2.5 * PI / 180.0)))
		{
		  chooseleft = false;
		  break;
		}
	    }
	  
	  // if we can't find a path within 90 degrees
	  // go straight towards our destination
	  if (rotationangle > 100.0 * PI / 180.0)
	    {
	      leftdirection = rightdirection = direction;
	      stop = true;
	      break;
	    }
	  rotationangle += 1.0 * PI / 180.0;//rotate by 1 degree each
					    //time
	}
      undiverted = rotationangle < 1e-5;
      if(stop)
	{
	  point balldest = the_ball->position() - the_player->position();
	  the_player->move(the_player->position(), atan2(balldest.y, balldest.x));
	}
      else
	{
	  point selected_direction;
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
	      point balldest = the_ball->position() - the_player->position();
	      the_player->move(nowDest, atan2(balldest.y, balldest.x));
	    }
	  else
	    {
	      // maximum warp
	      point balldest = the_ball->position() - the_player->position();
	      the_player->move(the_player->position() + selected_direction*std::min(dirlen,1.0), atan2(balldest.y, balldest.x));
	    }
	}
    }
}

void robot_navigator::set_point(const point &destination) {
  //set new destinatin point
  destInitialized = true;
  /*currDest = clip_point(destination,
			point(-the_field->length()/2,-the_field->width()/2),
			point(the_field->length()/2,the_field->width()/2));*/
  currDest = destination;
}

void robot_navigator::set_correction_step_size(double correction_size) {

}

void robot_navigator::set_desired_robot_orientation(double orientation) {

}

bool robot_navigator::robot_avoids_ball() {
return avoid_ball;
}

void robot_navigator::set_robot_avoids_ball(bool avoid){
avoid_ball=avoid;
}

bool robot_navigator::robot_avoids_goal() {
return false;
}

void robot_navigator::set_robot_avoids_goal(bool avoid) {

}

bool robot_navigator::robot_stays_on_own_half() {
return false;
}

void robot_navigator::set_robot_stays_on_own_half(bool avoid) {

}

bool robot_navigator::robot_stays_away_from_opponent_goal() {
return false;
}

void robot_navigator::set_robot_stays_away_from_opponent_goal(bool avoid) {

}

void robot_navigator::set_robot_avoid_opponent_goal_amount(double amount) {

}

bool robot_navigator::robot_is_in_desired_state() {
return false;
}

bool robot_navigator::robot_is_in_desired_location() {
return false;
}

bool robot_navigator::robot_is_in_desired_orientation(){
return false;
}

point robot_navigator::clip_point(point p, point bound1, point bound2)
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

bool robot_navigator::check_vector(point start, point dest, point direction)
{

  point startdest = dest - start;

  double lookahead = std::min(startdest.len(), maxLookahead);

  for (size_t i = 0; i < the_team->size() + the_team->other()->size(); i++)
    {
      robot::ptr rob;

      if (i >= the_team->size())
	{
	  rob = the_team->other()->get_robot(i - the_team->size());
	}
      else
	{
	  rob = the_team->get_robot(i);
	}
      
      if(rob != this->the_player)
	{
	  point rp = rob->position() - start;
	  //rp/= rp.len();
	  double len = rp.dot(direction);

	  // ignore robots behind us
	  if (len > 0)
	    {
	      double d = sqrt(rp.dot(rp) - len*len);
	  
	      if (len < lookahead && d < 2*aggression_factor*robot::MAX_RADIUS)
		{
		  //std::cout << "Checked FALSE" << std::endl;
		  return false;
		}
	    }
	}
    }
    
if(avoid_ball){
//check ball
point ballVec = the_ball->position() - start;
double len = ballVec.dot(direction);
	  if (len > 0)
	    {
	      double d = sqrt(ballVec.dot(ballVec) - len*len);
	  
	      if (len < lookahead && d < aggression_factor*robot::MAX_RADIUS)
		{
		  //std::cout << "Checked FALSE" << std::endl;
		  return false;
		}
	    }

}

  //std::cout << "Checked TRUE" << std::endl;
  return true;

}
