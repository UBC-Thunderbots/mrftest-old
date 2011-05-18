#ifndef AI_HL_STP_EVALUATION_CM_EVALUATION_H
#define AI_HL_STP_EVALUATION_CM_EVALUATION_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "util/cacheable.h"
#include "util/param.h"
#include "ai/hl/stp/cm_coordinate.h"

#include <vector>

#define MAX_TEAM_ROBOTS 5

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

#define P_DefendLookahead 1.0
#define P_DefendLookstep  FRAME_PERIOD

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				
				/**
				 * HL World evaluation ported from CMDragon world.cc
				 */
				namespace CMEval{

					int sideBall(World &world);

					int sideStrong(World &world);

					int sideBallOrStrong(World &world);

					/**
					 * Finds the nearest teammate to a point on the field.
					 */
					int nearest_teammate(World &world, Point p, double time);

					/**
					 * Finds the nearest opponent to a point on the field.
					 */
					int nearest_opponent(World &world, Point p, double time);

					/**
					 * Obs methods return an obs_flag set to why a position or other
  					 * shape is not open.  Or zero if the position or shape is open
					 */
					int obsPosition(World &world, Point p, int obs_flags, double pradius, double time = -1);

					int obsLine(World &world, Point p1, Point p2, int obs_flags, double pradius, double time);

					int obsLineFirst(World &world, Point p1, Point p2, int obs_flags, Point &first, double pradius, double time = -1);

					/**
					 * returns number of obstacles on the line
					 */
					int obsLineNum(World &world, Point p1, Point p2, int obs_flags, double pradius, double time = -1);

					/**
					 * returns true if point p will block a shot at time 
					 */
					bool obsBlocksShot(World &world, Point p, double time);
				}
				
				/**
				 * Evaluation functions ported from CMDragon evaluation.cc
				 */
				class CMEvaluation {
					public:
						/**
						 * aim()
					  	 *
						 * These functions take a target point and two relative vectors
						 * denoting the range to aim along.  They take an obs_flags of
						 * obstacles to avoid in aiming and then return the the point along
						 * the vectors with the largest clear angle.
						 *
						 * The pref_target_point provides a preferred direction along with a
						 * bias.  If no other direction is clear by more than the bias it
						 * will simply return the point along the largest open angle near
						 * the preference.  Used for hysteresis.
						 *
						 * aim() should be guaranteed not to return false if obs_flags is 0.
						 *
						 */
					  	bool aim(World &world, double time, Point target, Point r2, Point r1, int obs_flags, Point pref_target_point, double pref_amount, Point &target_point, double &target_tolerance);

						/**
						 * aim() but with pref_target_point set to center of the two aiming vectors and obs_flags set to 0
						 */
					  	bool aim(World &world, double time, Point target, Point r2, Point r1, int obs_flags, Point &target_point, double &target_tolerance) {
					    		return aim(world, time, target, r2, r1, obs_flags, ((r2 + r1) / 2.0), 0.0, target_point, target_tolerance);
					  	}
						/**
					  	 * defend_line()
					  	 * defend_point()
					 	 * defend_on_line()
					  	 *
					  	 * This returns the position and velocity to use to defend a
					  	 * particular line (or point) on the field.  It combines the
						 * positions of the best static defense with the interception point
						 * using the variance on the interception point from the Kalman
						 * filter.  It also computes the velocity to hit that point with.
						 *
						 * Setting obs_flags and optionally pref_point and pref_amount can
						 * be used to take account for other robots also defending.  When
						 * computing a static position it will use aim() with the provided
						 * paramters to find the largest remaining open angle and statically
						 * defend this range.
						 *
						 * The defend_*_static() and defend_*_trajectory() methods are
						 * helper functions that defend_*() uses.
						 *
						 * defend_on_line() positions itself along a line segment nearest to
						 * the ball.  The intercept flag here still works as biasing the 
						 * position towards where the ball will cross the segment.
						 *
						 * The intercept field specifies whether a moving ball should be
						 * intercepted.  If true after the call it means the robot is actively
						 * trying to intercept the ball.
						 *
						 */
					  	bool defend_line(World &world, double time, Point g1, Point g2, double distmin, double distmax, double dist_off_ball, bool &intercept, int obs_flags, Point pref_point, double pref_amount, Point &target, Point &velocity);

					  	bool defend_line(World &world, double time, Point g1, Point g2, double distmin, double distmax, double dist_off_ball, bool &intercept, Point &target, Point &velocity) {
					    		return defend_line(world, time, g1, g2, distmin, distmax, dist_off_ball, intercept, 0, Point(), 0.0, target, velocity);
					  	}
						
					  	bool defend_point(World &world, double time, Point point, double distmin, double distmax, double dist_off_ball, bool &intercept, Point &target, Point &velocity);

					  	bool defend_on_line(World &world, double time, Point p1, Point p2, bool &intercept, Point &target, Point &velocity);
					  
					private:
						/**
						 * defend_line_static()
						 *
						 * (g1, g2) defines a line segment to be defended.
						 *
						 * p1 is the point where the ball shot at g1 crosses the desired line.
						 * p2 is the point where the ball shot at g2 crosses the desired line.
						 *
						 * d1 is the distance from the ball to p1.
						 * d2 is the distance from the ball to p2.
						 *
						 * y is the distance between p1 and p2.
						 * x is the distance from p1 to the target point.
						 */
					  	bool defend_line_static(World &world, double time, Point g1, Point g2, double dist, Point &target, double &variance);
						/**
						 * defend_line_intercept()
						 *
						 * (g1, g2) defines a line segment to be defended.
						 *
						 * We lookahead through the ball's trajectory to find where, if it at
						 * all the ball crosses the goalie's line.  The covariance matrix is then
						 * used to set the variance for this position.
						 */
					  	bool defend_line_intercept(World &world, double time, Point g1, Point g2, double dist, Point &target, double &variance);
						/**
						 * defend point helper function
						 */
					  	bool defend_point_static(World &world, double time, Point point, double radius, Point &target, double &variance);

					  	bool defend_point_intercept(World &world, double time, Point point, double radius, Point &target, double &variance);

					public:
						/**
						 * finds the furthest point of a robot in a direction
						 */
					  	Point farthest(World &world, double time, int obs_flags, Point bbox_min, Point bbox_max, Point dir);
						
						/**
						 * finds an open position
						 */
					  	Point findOpenPosition(World &world, Point p, Point toward, int obs_flags, double pradius = Robot::MAX_RADIUS);
						/**
						 * finds an open position and yield
						 */
					  	Point findOpenPositionAndYield(World &world, Point p, Point toward, int obs_flags);
				};

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
					  	unsigned int n_points;
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
					    		set(_region, _eval, _pref_amount, _n_points);
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
					    		Only update if world has changed.
					    		if (world.time <= last_updated) return;
					    		last_updated = world.time;
							*/
					    		obs_flags = _obs_flags;

					    		// Check the new points to make sure their within the region.
						    	// Passed here by addPoint().
						    	for(unsigned int i = 0; i<new_points.size(); i++)
						      		if (!region.inRegion(world, new_points[i])) {
									new_points.erase(new_points.begin() + i);
									i--;
						      		}

					    		// Add previous best point (or center).
					    		if (!points.empty()) {
					      			new_points.push_back(points[best]); 
								best = static_cast<int> (new_points.size()) - 1; 
					    		} else {
					      			new_points.push_back(region.center(world)); 
								best = -1;
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
						    	for(unsigned int i=0; i<points.size(); i++) {
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
					    		for(unsigned int i=0; i<new_points.size(); i++)
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

