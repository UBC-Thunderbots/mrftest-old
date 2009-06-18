// RobotAlgorithmUtils: utilities for doing geometry and calculation
// Such as Hungarian matching and finding where a goalkeeper should be

#ifndef AI_ROBOTALGORITHMUTILS_H
#define AI_ROBOTALGORITHMUTILS_H

#include "datapool/Vector2.h"

// WARNING: does not work with parallel lines
// finds the intersection of 2 non-parallel lines
Vector2 line_intersect(const Vector2 &a, const Vector2 &b, const Vector2 &c, const Vector2 &d);

// WARNING: does not work with parallel lines
// there is a ray that shoots out from origin
// the ray is bounded by direction vectors a and b
// want to block this ray with circle of radius r
// where to position the circle?
Vector2 calc_block_ray(const Vector2 &a, const Vector2 &b, const double radius);

// WARNING: output is SIGNED indicating clockwise/counterclockwise direction
// signed line-point distance
double line_point_dist(const Vector2 &p, const Vector2 &a, const Vector2 &b);

// tests if 2 line segments crosses each other
bool seg_crosses_seg(const Vector2 &a1, const Vector2 &a2, const Vector2 &b1, const Vector2 &b2);

// gets a player with the ball
// returns the index
// returns -1 if no such player exist
// int get_player_with_ball(const AITeam& team);

// gets a player nearest to the point
// mask indicates which player to ignore
// returns an index
// int get_nearest_player(const Vector2& P, const AITeam& team, int mask = 0);

// a = goal post position
// b = other goal post position
// c = ball position
// the goalie will move tangent to a
// R = radius of goalie defence circle
// r = radius of robot
Vector2 calcBlockGoalie(const Vector2& a, const Vector2& b, const Vector2& c,
	double R, double r);

// reflects the ray r incident on origin, with normal n
Vector2 reflect(const Vector2&v, const Vector2& n);

// a = goal post position
// c = ball position
// g = goalie position
// returns the other ray that is not blocked by goalie
Vector2 calcBlockOtherRay(const Vector2& a, const Vector2& c, const Vector2& g);

// a = goal post position
// b = other goal post position
// c = ball position
// g = goalie position
// checks if goalie blocks goal post
bool goalieBlocksGoalPost(const Vector2& a, const Vector2& b, const Vector2& c, const Vector2& g);

// a = goal post position
// b = other goal post position
// c = ball position
// g = goalie position
// finds a defender position to block the ball
Vector2 defenderBlocksGoalPost(const Vector2& a, const Vector2& b, const Vector2& c, const Vector2& g, double r);

#endif

