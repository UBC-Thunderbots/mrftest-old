#ifndef AI_NAVIGATOR_TESTNAVIGATOR_H
#define AI_NAVIGATOR_TESTNAVIGATOR_H

#include "ai/navigator.h"
#include "world/field.h"
class testnavigator : public navigator {
 public:
  testnavigator(player::ptr player, field::ptr field);
  void update();
  void go_to_point(const point& destination);
 private:
  point clip_point(point p, point bound1, point bound2);

  bool destInitialized;//has a destination been specified?
  point currDest;//current destination
  float outOfBoundsMargin;//distance to remain from sidelines to prevent from going oob
};

#endif

