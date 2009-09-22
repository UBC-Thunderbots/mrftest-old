#ifndef TESTNAVIGATOR_H
#define TESTNAVIGATOR_H

#include "ai/navigator.h"

class testnavigator : public navigator
{
 public:

  testnavigator(player::ptr player);

  void update();

  void go_to_point(const point& destination);

};

#endif
