#ifndef AI_NAVIGATOR_TESTNAVIGATOR_H
#define AI_NAVIGATOR_TESTNAVIGATOR_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"

class TestNavigator : public NonCopyable {
 public:
  TestNavigator(Player::Ptr player, World::Ptr world);
  void tick();
  void set_point(const Point& destination);
 private:
  Point clip_point(Point p, Point bound1, Point bound2);
  bool check_vector(Point start, Point dest, Point direction);

  const Player::Ptr the_player;
  const World::Ptr the_world;
  bool destInitialized;//has a destination been specified?
  Point currDest;//current destination
  float outOfBoundsMargin;//distance to remain from sidelines to prevent from going oob
  double maxLookahead;
  double aggression_factor; //smaller factor makes robot more aggressive (i.e. less eager to avoid obstacles)
};

#endif

