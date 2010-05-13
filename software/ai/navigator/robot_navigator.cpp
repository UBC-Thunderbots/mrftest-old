#include "ai/navigator/robot_navigator.h"
#include <iostream>
#include <cstdlib>

  const double SLOW_AVOIDANCE_SPEED=1.0;
  const double FAST_AVOIDANCE_SPEED=2.0;

robot_navigator::robot_navigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team) : 
  navigator(player, field, ball, team), destInitialized(false), outOfBoundsMargin(field->width() / 20.0),
  maxLookahead(1.0), avoidance_factor(2), slow_avoidance_factor(0.5), fast_avoidance_factor(2.0)
{

}

double robot_navigator::get_avoidance_factor(){

double robot_speed = the_player->est_velocity().len();

if(robot_speed < SLOW_AVOIDANCE_SPEED){
return get_slow_avoidance_factor();
}

if(robot_speed > FAST_AVOIDANCE_SPEED){
return get_fast_avoidance_factor();
}

assert(SLOW_AVOIDANCE_SPEED < FAST_AVOIDANCE_SPEED);

double slowness = (robot_speed - FAST_AVOIDANCE_SPEED)/(SLOW_AVOIDANCE_SPEED - FAST_AVOIDANCE_SPEED);
double fastness = ( SLOW_AVOIDANCE_SPEED - robot_speed)/(SLOW_AVOIDANCE_SPEED - FAST_AVOIDANCE_SPEED);

return get_slow_avoidance_factor()*slowness + get_fast_avoidance_factor()*fastness;

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

/**
intended to specify how far robot should travel after making a path correction
not final! 
implement or delete function soon

 \param correction_size - amount of distance travelled to correct path
 
**/
void robot_navigator::set_correction_step_size(double) {
#warning "implement or delete function soon"
}
/**
normally the navigator sets the robot orientation to be towards the ball
use this if you want to override this behaviour
this only sets the desired orientation for one timestep
\param orientation
*/
void robot_navigator::set_desired_robot_orientation(double) {
#warning "implement function"
}

bool robot_navigator::robot_avoids_ball() {
return avoid_ball;
}

void robot_navigator::set_robot_avoids_ball(bool avoid){
avoid_ball=avoid;
}

/**
get whether the robot avoid it's own goal
*/
bool robot_navigator::robot_avoids_goal() {
#warning "implement or delete function soon"
return false;
}

/**
make the robot avoid it's own goal
\param avoid whether to avoid it's own goal
*/
void robot_navigator::set_robot_avoids_goal(bool) {
#warning "implement or delete function soon"
}

/**
get whether the robot is set to stay on it's own half
*/
bool robot_navigator::robot_stays_on_own_half() {
#warning "implement function"
return false;
}

/**
make the robot stay on it's own half
\param avoid whether to make robot stay on it's own half
*/
void robot_navigator::set_robot_stays_on_own_half(bool) {
#warning "implement function"
}

/**
get whether the robot avoid it's opponents goal
*/
bool robot_navigator::robot_stays_away_from_opponent_goal() {
#warning "implement function"
return false;
}

/**
make the robot avoid it's opponents goal
\param avoid whether to avoid it's opponents goal
*/
void robot_navigator::set_robot_stays_away_from_opponent_goal(bool) {
#warning "implement function"
}

/**
set how much the robot should avoid the opponents goal by
\param amount the amount that the robot should avoid goal by
*/
void robot_navigator::set_robot_avoid_opponent_goal_amount(double) {
#warning "implement function"
}

bool robot_navigator::robot_is_in_desired_state() {
#warning "implement function"
return false;
}

bool robot_navigator::robot_is_in_desired_location() {
#warning "implement function"
return false;
}

bool robot_navigator::robot_is_in_desired_orientation(){
#warning "implement function"
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
	  
	      if (len < lookahead && d < 2*get_avoidance_factor()*robot::MAX_RADIUS)
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
	  
	      if (len < lookahead && d < get_avoidance_factor()*robot::MAX_RADIUS)
		{
		  //std::cout << "Checked FALSE" << std::endl;
		  return false;
		}
	    }

}

  //std::cout << "Checked TRUE" << std::endl;
  return true;

}
