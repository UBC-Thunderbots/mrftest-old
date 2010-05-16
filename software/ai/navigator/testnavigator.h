#ifndef AI_NAVIGATOR_TESTNAVIGATOR_H
#define AI_NAVIGATOR_TESTNAVIGATOR_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"

class testnavigator : public noncopyable {
 public:
  testnavigator(player::ptr player, world::ptr world);
  void tick();
  void set_point(const point& destination);
 private:
  point clip_point(point p, point bound1, point bound2);
  bool check_vector(point start, point dest, point direction);

  const player::ptr the_player;
  const world::ptr the_world;
  bool destInitialized;//has a destination been specified?
  point currDest;//current destination
  float outOfBoundsMargin;//distance to remain from sidelines to prevent from going oob
  double maxLookahead;
  double aggression_factor; //smaller factor makes robot more aggressive (i.e. less eager to avoid obstacles)
};

#endif

