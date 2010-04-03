#ifndef AI_NAVIGATOR_TESTNAVIGATOR_H
#define AI_NAVIGATOR_TESTNAVIGATOR_H

#include "ai/navigator.h"

class testnavigator : public navigator {
 public:
  testnavigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team);
  void tick();
  void set_point(const point& destination);
 private:
  point clip_point(point p, point bound1, point bound2);
  bool check_vector(point start, point dest, point direction);

  bool destInitialized;//has a destination been specified?
  point currDest;//current destination
  float outOfBoundsMargin;//distance to remain from sidelines to prevent from going oob
  double maxLookahead;
  double aggression_factor; //smaller factor makes robot more aggressive (i.e. less eager to avoid obstacles)
};

#endif

