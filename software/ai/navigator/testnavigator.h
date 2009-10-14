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
  bool destInitialized;//has a destination been specified?
  point currDest;//current destination
};

#endif

