#ifndef AI_NAVIGATOR_TESTNAVIGATOR_H
#define AI_NAVIGATOR_TESTNAVIGATOR_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"

class collisionnavigator : public noncopyable {
 public:
  collisionnavigator(player::ptr player, world::ptr world);
  void tick();
  void set_point(const point& destination);
 private:
  point clip_point(point p, point bound1, point bound2);

  const player::ptr the_player;
  const world::ptr the_world;
  bool destInitialized;//has a destination been specified?
  point currDest;//current destination
  float outOfBoundsMargin;//distance to remain from sidelines to prevent from going oob
  double maxLookahead;
};

#endif

