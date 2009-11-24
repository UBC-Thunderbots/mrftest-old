#ifndef AI_NAVIGATOR_TESTNAVIGATOR_H
#define AI_NAVIGATOR_TESTNAVIGATOR_H

#include "ai/navigator.h"

class collisionnavigator : public navigator {
 public:
  collisionnavigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team);
  void tick();
  void set_point(const point& destination);
 private:
  point clip_point(point p, point bound1, point bound2);

  bool destInitialized;//has a destination been specified?
  point currDest;//current destination
  float outOfBoundsMargin;//distance to remain from sidelines to prevent from going oob
  double maxLookahead;
};

#endif

