#ifndef AI_NAVIGATOR_TESTNAVIGATOR_H
#define AI_NAVIGATOR_TESTNAVIGATOR_H

#include "ai/navigator.h"

class robot_navigator : public navigator {
 public:
typedef Glib::RefPtr<robot_navigator> ptr;
  robot_navigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team);
  
  void tick();
  void set_point(const point& destination);

  //set the amount of avoidance 
  void set_slow_avoidance_factor(double factor){
  slow_avoidance_factor=factor;
  }
  
  //get the amount of avoidance 
  double get_slow_avoidance_factor(){ 
  return slow_avoidance_factor;
  }

  //set the amount of avoidance 
  void set_fast_avoidance_factor(double factor){
  fast_avoidance_factor = factor;
  }

  //get the amount of avoidance 
  double get_fast_avoidance_factor(){
  return fast_avoidance_factor;
  }
  
  //get the agression factor that is associateds with the given speed
  double get_avoidance_factor();

  void set_correction_step_size(double correction_size);  
  void set_desired_robot_orientation(double orientation);
  
  void set_robot_avoid_ball_amount(int amount);   
  bool robot_avoids_ball();
  void set_robot_avoids_ball(bool avoid);
  
  bool robot_avoids_goal();
  void set_robot_avoids_goal(bool avoid);
  
  bool robot_stays_on_own_half();
  void set_robot_stays_on_own_half(bool avoid);

  void set_robot_avoid_opponent_goal_amount(double amount);
  bool robot_stays_away_from_opponent_goal();
  void set_robot_stays_away_from_opponent_goal(bool avoid);
  
  bool robot_is_in_desired_state();
  bool robot_is_in_desired_location();
  bool robot_is_in_desired_orientation();


    
 private:
  point clip_point(point p, point bound1, point bound2);
  bool check_vector(point start, point dest, point direction);

  bool destInitialized;//has a destination been specified?
  point currDest;//current destination
  float outOfBoundsMargin;//distance to remain from sidelines to prevent from going oob
  double maxLookahead;
  double avoidance_factor; //smaller factor makes robot more aggressive (i.e. less eager to avoid obstacles)
  double slow_avoidance_factor;
  double fast_avoidance_factor; 
  bool avoid_ball;
  bool avoids_goal;
  double avoid_goal_amount;
  bool robot_stays_on_half;
  bool robot_avoids_opponent_goal;
  double avoid_opponent_goal_amount;
  
};

#endif

