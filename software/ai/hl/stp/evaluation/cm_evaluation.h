#ifndef AI_HL_STP_EVALUATION_CM_EVALUATION_H
#define AI_HL_STP_EVALUATION_CM_EVALUATION_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "util/cacheable.h"
#include "util/param.h"
#include "ai/hl/stp/cm_coordinate.h"

#include <set>
#include <utility>
#include <vector>

#define MAX_ROBOTS      10
#define MAX_TEAM_ROBOTS  5

#define MAX_ROBOT_ID    16
#define MAX_ROBOT_ID_OURS   10

#define TEAM_BLUE   0
#define TEAM_YELLOW 1
#define TEAM_NONE   2
#define NUM_TEAMS   2

#define SIDE_LEFT   1
#define SIDE_RIGHT -1

/* robot types */
#define ROBOT_TYPE_NONE   0
#define ROBOT_TYPE_DIFF   1
#define ROBOT_TYPE_OMNI   2
#define ROBOT_TYPE_GOALIE 3
#define NUM_ROBOT_TYPES   4

// check these!
#define DIFF_WHEELBASE_RADIUS 60.0
#define OMNI_WHEELBASE_RADIUS 60.0
/* OMNI_WHEELBASE_RADIUS is 73.0 from measurement */

/* macro for team identity */
#define IS_YELLOW(y, a, b)     ((y) ? (a) : (b))

/* macro to determine the robot type from the id */
#define ROBOT_TYPE_ID(id)      (((id) > 4) ? ROBOT_TYPE_OMNI : ROBOT_TYPE_DIFF)


//==== Field Dimensions (mm) =========================================//

// diagonal is of 2800 x 2300 is 3623.53
#define FIELD_LENGTH    2800 /* 2780 */
#define FIELD_WIDTH     2400 /* 2300 1525 */
#define FIELD_LENGTH_H  (FIELD_LENGTH /2)
#define FIELD_WIDTH_H   (FIELD_WIDTH  /2)

#define GOAL_WIDTH          600
#define GOAL_DEPTH          180
#define DEFENSE_WIDTH      1000
#define DEFENSE_DEPTH       225
#define WALL_WIDTH           50
#define CORNER_BLOCK_WIDTH   40

#define GOAL_WIDTH_H     (GOAL_WIDTH   /2)
#define GOAL_DEPTH_H     (GOAL_DEPTH   /2)
#define DEFENSE_WIDTH_H  (DEFENSE_WIDTH/2)
#define DEFENSE_DEPTH_H  (DEFENSE_DEPTH/2)

#define PENALTY_FROM_GOAL   (225)
#define PENALTY_SPOT        (FIELD_LENGTH_H - PENALTY_FROM_GOAL)

#define CENTER_CIRCLE_RADIUS (460/2)

#define FREEKICK_FROM_WALL 150
#define FREEKICK_FROM_GOAL 375

#define BALL_RADIUS      21
#define BALL_DIAMETER    (BALL_RADIUS * 2)

//==== Robot Dimensions (mm) =========================================//

#define ROBOT_DEF_WIDTH     180.0
#define ROBOT_DEF_WIDTH_H   (ROBOT_DEF_WIDTH / 2.0)

#define DIFFBOT_WIDTH       160.0
#define DIFFBOT_LENGTH      120.0
#define DIFFBOT_WIDTH_H     (DIFFBOT_WIDTH / 2.0)
#define DIFFBOT_LENGTH_H    (DIFFBOT_LENGTH / 2.0)
#define DIFFBOT_HEIGHT      100.0
#define DIFFBOT_RADIUS      100.0 // sqrt(width^2 + length^2)

#define OMNIBOT_RADIUS       90.0
#define OMNIBOT_HEIGHT      150.0

#define TEAMMATE_HEIGHT     100.0
#define OPPONENT_HEIGHT     100.0

#define TEAMMATE_EFFECTIVE_RADIUS  100.0
#define OPPONENT_EFFECTIVE_RADIUS  90.0

//==== Obstacle Flags ================================================//

// Standard Obstacles
#define OBS_BALL         (1 << 0)
#define OBS_WALLS        (1 << 1)
#define OBS_THEIR_DZONE  (1 << 2)
#define OBS_OUR_DZONE    (1 << 3)
#define OBS_TEAMMATE(id) (1 << (4 + id))
#define OBS_OPPONENT(id) (1 << (4 + MAX_TEAM_ROBOTS + id))
#define OBS_TEAMMATES    ( ((1 << MAX_TEAM_ROBOTS) - 1) << 4)
#define OBS_OPPONENTS    ( ((1 << MAX_TEAM_ROBOTS) - 1) << 4 + MAX_TEAM_ROBOTS)

#define OBS_EVERYTHING (~ ((int) 0))
#define OBS_EVERYTHING_BUT_ME(id) (OBS_EVERYTHING & (~(OBS_TEAMMATE(id))))
#define OBS_EVERYTHING_BUT_US (OBS_EVERYTHING & (~(OBS_TEAMMATES)))
#define OBS_EVERYTHING_BUT_BALL (OBS_EVERYTHING & (~(OBS_BALL)))

//==== Miscellaneous =================================================//

/* Frame time and latencies */
#define FRAME_RATE    15.0
#define FRAME_PERIOD  (1.0 / FRAME_RATE)

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				class CMEvaluation {
					public:
						// aim()
					  	//
						// These functions take a target point and two relative vectors
						// denoting the range to aim along.  They take an obs_flags of
						// obstacles to avoid in aiming and then return the the point along
						// the vectors with the largest clear angle.
						//
						// The pref_target_point provides a preferred direction along with a
						// bias.  If no other direction is clear by more than the bias it
						// will simply return the point along the largest open angle near
						// the preference.  Used for hysteresis.
						//
						// aim() should be guaranteed not to return false if obs_flags is 0.
						//

					  	bool aim(World &world, double time, Point target, Point r2, Point r1, int obs_flags,
						   	Point pref_target_point, double pref_amount, Point &target_point, double &target_tolerance);

					  	bool aim(World &world, double time, Point target, Point r2, Point r1, int obs_flags,
						   	Point &target_point, double &target_tolerance) {
					    		return aim(world, time, target, r2, r1, obs_flags, ((r2 + r1) / 2.0), 0.0,
						       		target_point, target_tolerance);
					  	}

					  	// defend_line()
					  	// defend_point()
					 	// defend_on_line()
					  	//
					  	// This returns the position and velocity to use to defend a
					  	// particular line (or point) on the field.  It combines the
						// positions of the best static defense with the interception point
						// using the variance on the interception point from the Kalman
						// filter.  It also computes the velocity to hit that point with.
						//
						// Setting obs_flags and optionally pref_point and pref_amount can
						// be used to take account for other robots also defending.  When
						// computing a static position it will use aim() with the provided
						// paramters to find the largest remaining open angle and statically
						// defend this range.
						//
						// The defend_*_static() and defend_*_trajectory() methods are
						// helper functions that defend_*() uses.
						//
						// defend_on_line() positions itself along a line segment nearest to
						// the ball.  The intercept flag here still works as biasing the 
						// position towards where the ball will cross the segment.
						//
						// The intercept field specifies whether a moving ball should be
						// intercepted.  If true after the call it means the robot is actively
						// trying to intercept the ball.
						//

					  	bool defend_line(World &world, double time, Point g1, Point g2, 
								double distmin, double distmax, double dist_off_ball,
							  	bool &intercept, int obs_flags, Point pref_point, double pref_amount,
							   	Point &target, Point &velocity);

					  	bool defend_line(World &world, double time, Point g1, Point g2, 
								double distmin, double distmax, double dist_off_ball,
							   	bool &intercept, Point &target, Point &velocity) {
					    		return defend_line(world, time, g1, g2, distmin, distmax, dist_off_ball,
							       intercept, 0, Point(), 0.0, target, velocity);
					  	}
						
					  	bool defend_point(World &world, double time, Point point, 
							    double distmin, double distmax, double dist_off_ball,
							    bool &intercept, Point &target, Point &velocity);

					  	bool defend_on_line(World &world, double time, Point p1, Point p2,
							      bool &intercept, Point &target, Point &velocity);
					  
					private:
					  	bool defend_line_static(World &world, double time, Point g1, Point g2, double dist,
								  	Point &target, double &variance);

					  	bool defend_line_intercept(World &world, double time, Point g1, Point g2, double dist,
								     	Point &target, double &variance);

					  	bool defend_point_static(World &world, double time, Point point, double radius,
								   	Point &target, double &variance);

					  	bool defend_point_intercept(World &world, double time, Point point, double radius,
								      	Point &target, double &variance);

					public:
					  	Point farthest(World &world, double time, int obs_flags, Point bbox_min, Point bbox_max, Point dir);

					  	Point findOpenPosition(World &world, Point p, Point toward, int obs_flags,
								   	double pradius = TEAMMATE_EFFECTIVE_RADIUS);

					  	Point findOpenPositionAndYield(World &world, Point p, Point toward, int obs_flags);
				};

				extern CMEvaluation evaluation;

				class CMEvaluationPosition {
					public:
					  	typedef double (*EvalFn)(World &world, const Point p, int obs_flags, double &a);

					  	TRegion region;

					private:
					  	// Function
					  	EvalFn eval;

					  	int obs_flags;

					  	double last_updated;

					  	// Evaluated Points
					  	uint n_points;
					  	std::vector<Point> points;
					  	std::vector<double> angles;
					  	std::vector<double> weights;

					  	std::vector<Point> new_points;

					  	int best;
					  	double pref_amount;

					  	Point pointFromDistribution(World &w) {
					    		return region.sample(w);
					  	}

					public:
					  	CMEvaluationPosition() { }
					  	CMEvaluationPosition(TRegion _region, EvalFn _eval, double _pref_amount = 0, int _n_points = 10) {
					    		set(_region, _eval, _pref_amount, n_points);
					  	}

					  	void set(TRegion _region, EvalFn _eval, double _pref_amount = 0, int _n_points = 10) {
					    		region = _region;
					    		eval = _eval; 
							n_points = _n_points;
					    		pref_amount = _pref_amount;

					    		obs_flags = 0;

					    		points.clear();
					    		weights.clear();
					    		best = -1;
					    		last_updated = 0;
					  	}
						
					  	void update(World &world, int _obs_flags) {
							/*
					    		// Only update if world has changed.
					    		if (world.time <= last_updated) return;
					    		last_updated = world.time;
							*/
					    		obs_flags = _obs_flags;

					    		// Check the new points to make sure their within the region.
						    	// Passed here by addPoint().
						    	for(uint i = 0; i<new_points.size(); i++)
						      		if (!region.inRegion(world, new_points[i])) {
									new_points.erase(new_points.begin() + i);
									i--;
						      		}

					    		// Add previous best point (or center).
					    		if (!points.empty()) {
					      			new_points.push_back(points[best]); best = new_points.size() - 1; 
					    		} else {
					      			new_points.push_back(region.center(world)); best = -1;
					    		}

					    		// Pick new points.
					    		while(new_points.size() < n_points)
					      			new_points.push_back(pointFromDistribution(world));

					    		points = new_points;
					    		new_points.clear();
					      
					    		// Evaluate points.
					    		int best_i = -1;

					    		weights.clear(); 
							angles.clear();
						    	for(uint i=0; i<points.size(); i++) {
						      		double a;

						      		weights.push_back((eval)(world, points[i], obs_flags, a));
						      		angles.push_back(a);

						      		if (best_i < 0 || weights[i] > weights[best_i]) best_i = i;
						    	}

						    	if (best < 0 || weights[best_i] > weights[best] + pref_amount) {
						      		best = best_i;
						    	}

					  	}
						
					  	void addPoint(Point p) {
					    		for(uint i=0; i<new_points.size(); i++)
					      			if (new_points[i].x == p.x && new_points[i].y == p.y) return;
					    		new_points.push_back(p); 
						}

					  	Point point() const {
					    		return points[best]; 
						}

					  	double angle() const {
					    		return angles[best]; 
						}
				};

			}
		}
	}
}

#endif

