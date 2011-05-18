#include "ai/hl/stp/evaluation/cm_evaluation.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
//#include "util/cm_matrix.h"
#include "util/kalman/kalman.h"
#include "geom/util.h"
#include "geom/cm_util.h"
#include "util/dprint.h"
#include "util/param.h"
#include "util/time.h"

using namespace AI::HL::W;

namespace {

	Kalman ball_kalman = Kalman(4, 2, FRAME_PERIOD); // Kalman for the ball

	/**
  	 * CMDragons Obstacle Computations
	 */

	Point our_goal = Point(-FIELD_LENGTH_H, 0.0);
	Point our_goal_l = Point(-FIELD_LENGTH_H, GOAL_WIDTH_H);
	Point our_goal_r = Point(-FIELD_LENGTH_H, -GOAL_WIDTH_H);
	Point their_goal = -our_goal;
	Point their_goal_l = -our_goal_r;
	Point their_goal_r = -our_goal_l;

	// Finds the nearest teammate/opponent to a point on the field.

	int nearest_teammate(World &world, Point p, double time){
		int dist_i = -1;
	  	double dist = 0;

	  	for(int i=0; i<world.friendly_team().size(); i++) {
	    		double d = (p - world.friendly_team().get(i)->position(time)).len();
	    		if (dist_i < 0 || d < dist) {
	      			dist_i = i; dist = d;
	    		}
	  	}

	  	return dist_i;
	}

	int nearest_opponent(World &world, Point p, double time){
		int dist_i = -1;
	  	double dist = 0;

	  	for(int i=0; i<world.enemy_team().size(); i++) {
	    		double d = (p - world.enemy_team().get(i)->position(time)).len();
	    		if (dist_i < 0 || d < dist) {
	      			dist_i = i; dist = d;
	    		}
	  	}

	  	return dist_i;
	}

	// Obs methods return an obs_flag set to why a position or other
  	// shape is not open.  Or zero if the position or shape is open
  	//
  	// obsLineNum() returns the number of obstacles on the line.
  	//
  	// obsBlocksShot() returns wether a point would block a shot right
  	// now.

	int obsPosition(World &world, Point p, int obs_flags, double pradius, double time = -1){
	  	int rv = 0;

	  	// Teammates
	  	for(int i=0; i<world.friendly_team().size(); i++) { 
	    		if (!(obs_flags & OBS_TEAMMATE(i))) continue;

	    		double radius = TEAMMATE_EFFECTIVE_RADIUS + pradius; 

	    		if ((p - world.friendly_team().get(i)->position(time)).len() <= radius) 
	      			rv |= OBS_TEAMMATE(i);
	  	}

	  	// Opponents
	  	for(int i=0; i<world.enemy_team().size(); i++) {
	    		if (!(obs_flags & OBS_OPPONENT(i))) continue;

	    		double radius = OPPONENT_EFFECTIVE_RADIUS + pradius;

	    		if ((p - world.enemy_team().get(i)->position(time)).len() <= radius) 
	      			rv |= OBS_OPPONENT(i);
	  	}

	  	// Ball
	  	if (obs_flags & OBS_BALL) {
	    		double radius = BALL_RADIUS + pradius;
	    		if ((p - world.ball().position(time)).len() <= radius) 
	      			rv |= OBS_BALL;
	  	}

	  	// Walls
	  	if (obs_flags & OBS_WALLS) {
	    		double radius = pradius;
	    		if (fabs(p.x) + radius > FIELD_LENGTH_H || fabs(p.y) + radius > FIELD_WIDTH_H)
	      			rv |= OBS_BALL;
	  	}
	  
	  	// Defense Zones
	  	if (obs_flags & OBS_OUR_DZONE) {
	    		double radius = pradius;
	    		if (p.x <= -FIELD_LENGTH_H + DEFENSE_DEPTH + radius && fabs(p.y) <= DEFENSE_WIDTH_H + radius) 
	      			rv |= OBS_OUR_DZONE; 
	  	}

	  	if (obs_flags & OBS_THEIR_DZONE) {
	    		double radius = pradius;
	    		if (p.x >= FIELD_LENGTH_H - DEFENSE_DEPTH - radius && fabs(p.y) <= DEFENSE_WIDTH_H + radius) 
	      			rv |= OBS_THEIR_DZONE; 
	  	}

	  	// Nothing Left
	  	return rv;
	}

	int obsLine(World &world, Point p1, Point p2, int obs_flags, double pradius, double time){
	  	// Teammates
	  	for(int i=0; i<world.friendly_team().size(); i++) { 
	    		if (!(obs_flags & OBS_TEAMMATE(i))) continue;

	    		double radius = TEAMMATE_EFFECTIVE_RADIUS + pradius; 

	    		Point p = world.friendly_team().get(i)->position(time);
	    		if ((closest_lineseg_point(p1, p2, p) - p).len() > radius)
	      			obs_flags &= ~OBS_TEAMMATE(i);
	  	}

	  	// Opponents
	  	for(int i=0; i<world.enemy_team().size(); i++) {
	    		if (!(obs_flags & OBS_OPPONENT(i))) continue;

	    		double radius = OPPONENT_EFFECTIVE_RADIUS + pradius;

	    		Point p = world.enemy_team().get(i)->position(time);
	    		if ((closest_lineseg_point(p1, p2, p) - p).len() > radius)
	      		obs_flags &= ~OBS_OPPONENT(i);
	  	}

	  	// Ball
	  	if (obs_flags & OBS_BALL) {
	    		double radius = BALL_RADIUS + pradius;

	    		Point p = world.ball().position(time);
	    		if ((closest_lineseg_point(p1, p2, p) - p).len() > radius)
	      			obs_flags &= ~OBS_BALL;
	  	}

	  	// Walls
	  
	  	// Defense Zones

	  	// Nothing Left
	  	return obs_flags;
	}

	int obsLineFirst(World &world, Point p1, Point p2, int obs_flags, Point &first, double pradius, double time = -1){
	  	int rv = 0;

	  	first = p2;

	  	// Teammates
	  	for(int i=0; i<world.friendly_team().size(); i++) { 
	    		if (!(obs_flags & OBS_TEAMMATE(i))) continue;

	    		double radius = TEAMMATE_EFFECTIVE_RADIUS + pradius; 

	    		Point p = world.friendly_team().get(i)->position(time);
	    		Point pp = closest_lineseg_point(p1, first, p);
	    		double d = (pp - p).len();

	    		if (d < radius) {
		      		double dx = sqrt(radius * radius - d * d);
		      
		      		if ((p1 - pp).len() < dx) { 
					first = p1; return OBS_TEAMMATE(i); 
				} else {
					first = pp + (p1 - pp).norm(dx);
					rv = OBS_TEAMMATE(i);
		      		}
	    		}
	  	}

	  	// Opponents
	  	for(int i=0; i<world.enemy_team().size(); i++) {
	    		if (!(obs_flags & OBS_OPPONENT(i))) continue;

	    		double radius = OPPONENT_EFFECTIVE_RADIUS + pradius; 

	    		Point p = world.enemy_team().get(i)->position(time);
	    		Point pp = closest_lineseg_point(p1, first, p);
	    		double d = (pp - p).len();

	    		if (d < radius) {
	      			double dx = sqrt(radius * radius - d * d);
	      
	      			if ((p1 - pp).len() < dx) { 
					first = p1; return OBS_OPPONENT(i); 
				} else {
					first = pp + (p1 - pp).norm(dx);
					rv = OBS_OPPONENT(i);
	      			}
	    		}
	  	}

	  	// Ball
	  	if (obs_flags & OBS_BALL) {
	    		double radius = BALL_RADIUS + pradius;

	    		Point p = world.ball().position(time);;
	    		Point pp = closest_lineseg_point(p1, first, p);
	    		double d = (pp - p).len();

	    		if (d < radius) {
	      			double dx = sqrt(radius * radius - d * d);
	      
	      			if ((p1 - pp).len() < dx) { 
					first = p1; return OBS_BALL; 
				} else {
					first = pp + (p1 - pp).norm(dx);
					rv = OBS_BALL;
	      			}
	    		}
	  	}

	  	// Walls
	  
	  	// Defense Zones
	  	if (obs_flags & OBS_THEIR_DZONE) {
	    		if (obsPosition(world, p1, OBS_THEIR_DZONE, pradius, time)) {
	      			first = p1; return OBS_THEIR_DZONE;
	    		}

	    		Point i;

	    		i = intersection(p1, p2, Point(FIELD_LENGTH_H-DEFENSE_DEPTH-pradius, DEFENSE_WIDTH_H+pradius),
			     Point(FIELD_LENGTH_H-DEFENSE_DEPTH-pradius, -DEFENSE_WIDTH_H-pradius));
	    		if ((i - p1).dot(first - p1) > 0 && (i - first).dot(p1 - first) > 0) {
	      			first = i; rv = OBS_THEIR_DZONE;
	    		}

	    		i = intersection(p1, p2, Point(FIELD_LENGTH_H-DEFENSE_DEPTH-pradius, DEFENSE_WIDTH_H+pradius),
			     Point(FIELD_LENGTH_H, DEFENSE_WIDTH_H+pradius));
	    		if ((i - p1).dot(first - p1) > 0 && (i - first).dot(p1 - first) > 0) {
	      			first = i; rv = OBS_THEIR_DZONE;
	    		}

	    		i = intersection(p1, p2, Point(FIELD_LENGTH_H-DEFENSE_DEPTH-pradius, -DEFENSE_WIDTH_H-pradius),
			     Point(FIELD_LENGTH_H, -DEFENSE_WIDTH_H-pradius));
	    		if ((i - p1).dot(first - p1) > 0 && (i - first).dot(p1 - first) > 0) {
	      			first = i; rv = OBS_THEIR_DZONE;
	    		}
	  	}


	  	// Nothing Left
	  	return obs_flags;
	}

	int obsLineNum(World &world, Point p1, Point p2, int obs_flags, double pradius, double time = -1){
	  	int count = 0;

	  	// Teammates
	  	for(int i=0; i<world.friendly_team().size(); i++) { 
	    		if (!(obs_flags & OBS_TEAMMATE(i))) continue;

	    		double radius = TEAMMATE_EFFECTIVE_RADIUS + pradius; 

	    		Point p = world.friendly_team().get(i)->position(time);
	    		if ((closest_lineseg_point(p1, p2, p) - p).len() <= radius)
	      			count++;
	  	}

	  	// Opponents
	  	for(int i=0; i<world.enemy_team().size(); i++) {
	    		if (!(obs_flags & OBS_OPPONENT(i))) continue;

	    		double radius = OPPONENT_EFFECTIVE_RADIUS + pradius;

	    		Point p = world.enemy_team().get(i)->position(time);
	    		if ((closest_lineseg_point(p1, p2, p) - p).len() <= radius)
	      			count++;
	  	}

	  	// Ball
	  	if (obs_flags & OBS_BALL) {
	    		double radius = BALL_RADIUS + pradius;

	    		Point p = world.ball().position(time);
	    		if ((closest_lineseg_point(p1, p2, p) - p).len() <= radius)
	      			count++;
	  	}

	  	// Walls
	  
	  	// Defense Zones

	  	// Nothing Left
	  	return count;
	}

	bool obsBlocksShot(World &world, Point p, double pradius, double time){
	  	Point ball = world.ball().position(time);
		
	  	double a = (their_goal_r - ball).perp().dot(p - ball);
	  	double b = (their_goal_l - ball).perp().dot(p - ball);

	  	return (a * b) < 0;
	}

}

//CMEvaluation evaluation;

inline bool inside_bbox(Point bbox_min,Point bbox_max, Point p,double radius){
  	return((p.x+radius > bbox_min.x) && (p.y+radius > bbox_min.y) && (p.x-radius < bbox_max.x) && (p.y-radius < bbox_max.y));
}

static double diffangle_pos(double a1, double a2) {
  	double d = angle_mod(a1 - a2);
  	if (d < 0.0) d += M_2PI;
  	return d;
}

static int double_compare(const void *a, const void *b) {
  	return (*((double *) a) > *((double *) b));
}

bool AI::HL::STP::Evaluation::CMEvaluation::aim(World &world, double time, Point target, Point p2, Point p1, int obs_flags, Point pref_target_point, double pref_amount, Point &target_point, double &target_tolerance) {
  	static struct _double_int { double d; int i; } a[MAX_TEAM_ROBOTS * 4];
  	int n = 0, count = 0;
  	double a_zero;
  	double a_end;

  	Point r1 = p1 - target;
  	Point r2 = p2 - target;

  	// Swap sides of endpoints if the target cone is oblique.
  	if (diffangle_pos(r2.orientation(), r1.orientation()) > M_PI) {
    		Point t = r1;
    		r1 = r2;
    		r2 = t;
  	}

  	a_zero = r1.orientation();
  	a_end = diffangle_pos(r2.orientation(), a_zero);

  	double pref_target_angle = diffangle_pos((pref_target_point - target).orientation(), a_zero);
  	if (pref_target_angle - a_end > M_2PI - pref_target_angle)
    		pref_target_angle -= M_2PI;

  	a[n].d = 0.0; a[n++].i = 0; 
  	a[n].d = a_end; a[n++].i = 0;

  	for(int i=0; i<world.friendly_team().size(); i++) {
    		if (!(obs_flags & OBS_TEAMMATE(i))) continue;

    		double width = Robot::MAX_RADIUS;
    		Point obs = world.friendly_team().get(i)->position(time) - target;
    
    		Point obs_perp = obs.rotate(M_PI_2).norm() * width;
    
    		double a0 = diffangle_pos(obs.orientation(), a_zero);
    		double a1 = diffangle_pos((obs - obs_perp).orientation(), a_zero);
    		double a2 = diffangle_pos((obs + obs_perp).orientation(), a_zero);
    
    		double maxdist;
    
    		if (a0 < a_end)
      			maxdist = (a0 / a_end) * (r2.len() - r1.len()) + r1.len();
    		else {
      			if (a0 < (a_end + M_2PI) / 2.0) maxdist = r2.len();
      			else maxdist = r1.len();
    		}
    
    		if (obs.len() - width > maxdist) continue;
    
    		if (a1 < a_end) { a[n].d = a1; a[n++].i = 1; } 
    		if (a2 < a_end) { a[n].d = a2; a[n++].i = -1; }
    		if (a1 >= a_end && a2 < a_end) { count++; }
    		if (a1 >= a_end && a2 >= a_end && a1 > a2) { count++; }
  	}

  	double width = ROBOT_DEF_WIDTH_H;

  	for(int i=0; i<world.enemy_team().size(); i++) {
    		if (!(obs_flags & OBS_OPPONENT(i))) continue;

    		Point obs = world.enemy_team().get(i)->position(time) - target;
    		Point obs_perp = obs.rotate(M_PI_2).norm() * width;
    
    		double a0 = diffangle_pos(obs.orientation(), a_zero);
    		double a1 = diffangle_pos((obs - obs_perp).orientation(), a_zero);
    		double a2 = diffangle_pos((obs + obs_perp).orientation(), a_zero);
    
    		double maxdist;
    
    		if (a0 < a_end)
      			maxdist = (a0 / a_end) * (r2.len() - r1.len()) + r1.len();
    		else {
      			if (a0 < (a_end + M_2PI) / 2.0) maxdist = r2.len();
      			else maxdist = r1.len();
    		}
    
   		if (obs.len() - width > maxdist) continue;
    
    		if (a1 < a_end) { a[n].d = a1; a[n++].i = 1; } 
    		if (a2 < a_end) { a[n].d = a2; a[n++].i = -1; }
    		if (a1 >= a_end && a2 < a_end) { count++; }
    		if (a1 >= a_end && a2 >= a_end && a1 > a2) { count++; }
  	}

  	// Sort the angle array.
  	qsort(a, n, sizeof(a[0]), &double_compare);

  	// Walk through the angle array finding the largest clear cone, and
  	//  the closest clear cone to the preferred angle.
  	double best_ang = 0.0, best_tol = 0.0;
  	double closest_ang = M_2_PI, closest_tol = 0.0, closest_ang_diff = M_2_PI;
  	bool found_one = false;
  
  	for(int i=1; i<n; i++) {
    		if (!count) {
      			double tol = (a[i].d - a[i-1].d) / 2.0;
      			double ang = (a[i].d + a[i-1].d) / 2.0;
      			double ang_diff = std::max(0.0, fabs(angle_mod(ang - pref_target_angle))-tol);

      			if (!found_one || tol > best_tol) {
				best_tol = tol; best_ang = ang;
      			}
      
      			if (!found_one || ang_diff < closest_ang_diff) {
				closest_tol = tol; closest_ang = ang; closest_ang_diff = ang_diff;
      			}

      			found_one = true;
    		}
    
    		count += a[i].i;
  	}

  	// If there wasn't a clear angle we use the preferred angle and
  	// return false.  Otherwise we check whether to use the closest
  	// preferred or the largest depending on pref_amount and their
  	// angular difference.

  	double target_angle; 
  	bool rv;

  	if (found_one) {

	    	if (closest_tol + pref_amount > best_tol) {
	      		target_angle = closest_ang + a_zero;
	      		target_tolerance = closest_tol;
	    	} else {
	      		target_angle = best_ang + a_zero;
	      		target_tolerance = best_tol;
	    	}

    		rv = true;
  	} else {
    		target_angle = pref_target_angle + a_zero;
    		target_tolerance = 0.0;
    		rv = false;
  	}

  	target_point = intersection(p1, p2, target, target + Point(1, 0).rotate(target_angle));
  	// target_point += (target_point - target).norm(GOAL_DEPTH);

  	return rv;
}

bool AI::HL::STP::Evaluation::CMEvaluation::defend_point(World &world, double time, Point point, double distmin, double distmax, double dist_off_ball, bool &intercept, Point &target, Point &velocity){
  	double radius = (world.ball().position(time) - point).len() - dist_off_ball;

  	if (radius < distmin) return false;
  	if (radius > distmax) radius = distmax;

  	// We now can compute the static and intercept points and merge them.
  	// Index: 0 = intercept, 1 = static(now), 2 = static(future)
  	Point targets[3];
  	double variance[3];
  	int rv[3]; 

  	rv[0] = intercept && defend_point_intercept(world, time, point, radius,
					      targets[0], variance[0]);
  	intercept = rv[0];

  	rv[1] = defend_point_static(world, time, point, radius, targets[1], variance[1]);

  	rv[2] = defend_point_static(world, time + FRAME_PERIOD, point, radius, targets[2], variance[2]);

  	if (rv[0] && rv[1]) {
    		target = (targets[0] * variance[1] + targets[1] * variance[0]) / (variance[0] + variance[1]);
    		target = point + (target - point).norm(radius);

    		if (rv[2]) {
      			velocity = (targets[2] - targets[1]) / FRAME_PERIOD * variance[0] / (variance[0] + variance[1]);
    		} else velocity = Point(0, 0);

    		return true;
  	} else if (rv[0]) {
    		target = targets[0];

    		velocity = Point(0, 0);
    
    		return true;
  	} else if (rv[1]) {
    		target = targets[1];

	    	if (rv[2]) velocity = (targets[2] - targets[1]) / FRAME_PERIOD;
	    	else velocity = Point(0, 0);

	    	return true;
  	}


  	return true;
}

bool AI::HL::STP::Evaluation::CMEvaluation::defend_line(World &world, double time, Point g1, Point g2, double distmin, double distmax, double dist_off_ball, bool &intercept, int obs_flags, Point pref_point, double pref_amount, Point &target, Point &velocity){
  	Point ball = world.ball().position(time);
  	Point g = (g1 + g2) / 2.0;

  	Point gperp = (g2 - g1).perp().norm();
  	if (gperp.dot(ball - g) < 0) gperp *= -1;

  	// Special case of defending a single point. 
  	if (g1.x == g2.x && g1.y == g2.y) return defend_point(world, time, g1, distmin, distmax, dist_off_ball, intercept, target, velocity);

  	// First find the distance between min and max to play.
  	//
  	// The ratio is the cosine of the difference of the ball's angle to
  	// the center point and a perpendicular to the line.
  	// 
  	double ang = angle_mod((ball - g + gperp.norm(distmin)).orientation() - gperp.orientation());
  	double balldist = fabs(offset_to_line(g1, g2, ball));

  	double dist = distmin + fabs(cos(ang)) * (distmax - distmin);

  	if (dist > balldist - dist_off_ball) 
    		dist = balldist - dist_off_ball;

  	if (dist > distmax) dist = distmax;
  	if (dist < distmin) {
    		dist = distmin;
    		intercept = false;
  	}

  	// We now can compute the static and intercept points and merge them.
  	// Index: 0 = intercept, 1 = static(now), 2 = static(future)
  	Point targets[3];
  	double variance[3];
  	int rv[3]; 

  	rv[0] = intercept && defend_line_intercept(world, time, g1, g2, dist, targets[0], variance[0]);
  	intercept = rv[0];

  	if (!obs_flags) {
    		rv[1] = defend_line_static(world, time, g1, g2, dist, targets[1], variance[1]);
    		rv[2] = defend_line_static(world, time + FRAME_PERIOD, g1, g2, dist, targets[2], variance[2]);
  	} else {
    		Point p;
    		double tol;

    		rv[1] = rv[2] = false;

    		if (aim(world, time, ball, g1, g2, obs_flags, pref_point, pref_amount, p, tol)) {
      			Point ng1, ng2;
      
      			ng1 = intersection(ball, ball + (p - ball).rotate(tol), g1, g2);
      			ng2 = intersection(ball, ball + (p - ball).rotate(-tol), g1, g2);

      			rv[1] = defend_line_static(world, time, ng1, ng2, dist, targets[1], variance[1]);
      			rv[2] = defend_line_static(world, time + FRAME_PERIOD, ng1, ng2, dist, targets[2], variance[2]);
    		}
  	}


  	if (rv[0] && rv[1]) {
    		target = (targets[0] * variance[1] + targets[1] * variance[0]) / (variance[0] + variance[1]);

	    	if (rv[2]) {
	      		velocity = (targets[2] - targets[1]) / FRAME_PERIOD * variance[0] / (variance[0] + variance[1]);
	    	} else velocity = Point(0, 0);

	    	return true;
  	} else if (rv[0]) {
    		target = targets[0];

    		velocity = Point(0, 0);

    		return true; 
  	} else if (rv[1]) {
    		target = targets[1];

    		if (rv[2]) velocity = (targets[2] - targets[1]) / FRAME_PERIOD;
    		else velocity = Point(0, 0);

    		return true;
  	} else return false;
}

bool AI::HL::STP::Evaluation::CMEvaluation::defend_on_line(World &world, double time, Point p1, Point p2, bool &intercept, Point &target, Point &velocity){
  	Point ball = world.ball().position(time);
  	Point ball_dt = world.ball().position(time + FRAME_PERIOD);

  	Point targets[3];
  	double variance[2];

  	targets[1] = closest_lineseg_point(p1, p2, ball);
  	variance[1] = pow((p1 - p2).len(), 2.0) / 16;
  	targets[2] = closest_lineseg_point(p1, p2, ball_dt);

  	if (intercept && defend_line_intercept(world, time, p1, p2, 0.0, targets[0], variance[0])) {
    		target = (targets[0] * variance[1] + targets[1] * variance[0]) / (variance[0] + variance[1]);
    		velocity = (targets[2] - targets[1]) / FRAME_PERIOD * variance[0] / (variance[0] + variance[1]);
  	} else {
    		target = targets[1];
    		velocity = (targets[2] - targets[1]) / FRAME_PERIOD;
    		intercept = false;
  	}

  	return true;
}

// defend_line_static()
//
// (g1, g2) defines a line segment to be defended.
//
// p1 is the point where the ball shot at g1 crosses the desired line.
// p2 is the point where the ball shot at g2 crosses the desired line.
//
// d1 is the distance from the ball to p1.
// d2 is the distance from the ball to p2.
//
// y is the distance between p1 and p2.
// x is the distance from p1 to the target point.
//

bool AI::HL::STP::Evaluation::CMEvaluation::defend_line_static(World &world, double time, Point g1, Point g2, double dist, Point &target, double &variance){
  	Point g = (g1 + g2) / 2.0;
  	Point ball = world.ball().position(time);
  	double balldist = fabs(offset_to_line(g1, g2, ball));
  	double radius = 90.0; // should eventually be a parameter

  	double c1,c2;
  	double o1,o2;
  	double d1,d2;
  	double y;

  	Point gperp = (g2 - g1).perp().norm();
  	if (gperp.dot(ball - g) < 0) gperp *= -1;

  	if (balldist < dist + BALL_RADIUS) {
    		ball += gperp.norm(dist - balldist + BALL_RADIUS);
    		balldist = dist + BALL_RADIUS;
  	}

  	double ratio = dist / balldist;

  	Point p1 = ball * ratio + g1 * (1 - ratio);
  	Point p2 = ball * ratio + g2 * (1 - ratio);

  	// calculate inward offsets to take account of our radius
  	y = (p1-p2).len();
  	// c1 = fabs(cosine(ball-p1,p2-p1));
  	// c2 = fabs(cosine(ball-p2,p1-p2));
  	c1 = fabs(sin(vertex_angle(ball,p1,p2)));
  	c2 = fabs(sin(vertex_angle(ball,p2,p1)));
  	o1 = bound(radius / c1, 0.0, y/2);
  	o2 = bound(radius / c2, 0.0, y/2);

  	// correct the endpoint positions
  	p1 += (p2 - p1).norm(o1);
  	p2 += (p1 - p2).norm(o2);
  	y = (p1-p2).len();

  	/*
  	printf("%f %f : %f %f <%f,%f> <%f,%f> y=%f\n", c1, c2, o1, o2, V2COMP(p1), V2COMP(p2),y);
  	*/

  	// figure out where we want to be
  	if(y > 0.0){
    		d1 = (ball - p1).len();
    		d2 = (ball - p2).len();

    		double x = y * d2 / (d1 + d2);

    		target = p1 * (x / y) + p2 * (1 - x / y);
  	}else{
    		target = p1;
  	}
  	variance = y * y / 16; // (y/4)^2

  	return true;
}

/*
bool AI::HL::STP::Evaluation::CMEvaluation::defend_line_static(World &world, double time,
				    Point g1, Point g2, double dist,
				    Point &target, double &variance)
{
  Point ball = world.ball().position(time);
  double balldist = fabs(offset_to_line(g1, g2, ball));

  if (balldist < dist) return false;

  double ratio = dist / balldist;

  Point p1 = ball * ratio + g1 * (1 - ratio);
  Point p2 = ball * ratio + g2 * (1 - ratio);

  double y = (p1 - p2).len();

  double d1 = (ball - p1).len();
  double d2 = (ball - p2).len();

  double x = y * d2 / (d1 + d2);

  target = p1 * (x / y) + p2 * (1 - x / y);
  variance = y * y / 16; // (y/4)^2
  
  return true;
}
*/

// defend_line_intercept()
//
// (g1, g2) defines a line segment to be defended.
//
// We lookahead through the ball's trajectory to find where, if it at
// all the ball crosses the goalie's line.  The covariance matrix is then
// used to set the variance for this position.
//

#define P_DefendLookahead 1.0
#define P_DefendLookstep  FRAME_PERIOD

bool AI::HL::STP::Evaluation::CMEvaluation::defend_line_intercept(World &world, double time, Point g1, Point g2, double dist, Point &target, double &variance){
  	static double lookahead = P_DefendLookahead;
  	static double lookstep = P_DefendLookstep;
  	static double radius = 90.0; // Should be a parameter

  	Point gline = (g2 - g1);
  	Point gline_1 = gline.norm();
  	Point gperp = gline_1.rotate(M_PI_2);
  	Point ball = world.ball().position(time);

  	int side = (ball - g1).dot(gperp) >= 0.0 ? 1 : -1;
  
  	if (world.ball().velocity(time).dot(gperp) * side >= 0.0)
    		return false;

  	Point orig_g1 = g1, orig_g2 = g2;
  
  	//  g1 += gperp * (side * (dist + radius));
  	//  g2 += gperp * (side * (dist + radius));

  	g1 = intersection(ball, orig_g1, orig_g1 + gperp * (side * (dist + radius)), orig_g2 + gperp * (side * (dist + radius)));
  	g2 = intersection(ball, orig_g2, orig_g1 + gperp * (side * (dist + radius)), orig_g2 + gperp * (side * (dist + radius)));

  	// Lookahead from now to lookahead.
  	for(double t = 0.0; t < lookahead; t += lookstep) {

    		Point b = world.ball().position(time + t);
    		Point v = world.ball().velocity(time + t); 

    		if (v.dot(gperp) * side >= 0.0) return false;

    		double d_to_line = fabs((b - g1).dot(gperp));
    		double t_to_line = d_to_line / fabs(v.dot(gperp));

    		if (t_to_line > lookstep) continue;

    		b += v * t_to_line;

    		double x = offset_along_line(g1, g2, b);

    		Matrix c = ball_kalman.predict_cov(double_to_timespec(time + t));
    		Matrix m = Matrix(4,1); 
    		m(0,0) = gline_1.x;
	    	m(1,0) = gline_1.y;
	    	m(2,0) = m(3,0) = 0.0;
	    	Matrix temp = c * m;
		m.transpose();
		temp *= m;
		variance = temp(0,0);
    		//variance = (m.transpose() * c * m)(0,0);

    		if (x < 0.0) {
      			x = 0;
      			variance = variance * exp(pow(x, 2.0) / variance);
    		} else if (x > gline.len()) {
      			x = gline.len();
      			variance = variance * exp(pow(gline.len() - x, 2.0) / variance);
    		}

    		target = g1 + gline_1 * x;
    		return true;
  	}

  	return false;
}

bool AI::HL::STP::Evaluation::CMEvaluation::defend_point_static(World &world, double time, Point point, double radius, Point &target, double &variance){
  	Point ball = world.ball().position(time);
  	double ball_dist = (ball - point).len();

  	target = point + (ball - point).norm(radius);

  	double chordlength = (radius / ball_dist) * sqrt(ball_dist * ball_dist - radius * radius);

  	variance = chordlength * chordlength / 4.0;

  	return true;
}

bool AI::HL::STP::Evaluation::CMEvaluation::defend_point_intercept(World &world, double time, Point point, double radius, Point &target, double &variance){
  	static double lookahead = P_DefendLookahead;
  	static double lookstep = P_DefendLookstep;

  	Point ball = world.ball().position(time);
  	Point ball_vel = world.ball().velocity(time);

  	if (ball_vel.len() == 0.0 || ball_vel.dot(point - ball) < 0.0 || (ball - point).len() < radius) return false; 

  	// Lookahead from now to lookahead.
  	double closest_dist = (ball - point).len();
  	double closest_time = 0.0;

  	for(double t = 0.0; t < lookahead; t += lookstep) {
    		Point b = world.ball().position(time + t);

    		if ((b - point).len() < closest_dist) {
	      		closest_dist = (b - point).len();
	      		closest_time = t;
	      		target = point + (b - point).norm(radius);

	      		if (closest_dist < radius) break;
    		}
  	}

  	// Compute variance
  	Matrix c = ball_kalman.predict_cov(double_to_timespec(time + closest_time));
      
  	if (closest_dist > radius) {
    		Point perp = (target - point).norm();
    		Matrix m = Matrix(4,1); 
    		m(0,0) = perp.x;
    		m(1,0) = perp.y;
    		m(2,0) = m(3,0) = 0.0;
    		Matrix temp = c * m;
		m.transpose();
		temp *= m;
		variance = temp(0,0);
    		//variance = (m.transpose() * c * m)(0,0);

    		variance = variance * exp(pow(closest_dist - radius, 2.0) / variance);
  	} else {
    		Point perp = (target - point).perp().norm();
    		Matrix m = Matrix(4,1); 
    		m(0,0) = perp.x;
    		m(1,0) = perp.y;
    		m(2,0) = m(3,0) = 0.0;
		Matrix temp = c * m;
		m.transpose();
		temp *= m;
		variance = temp(0,0);
    		//variance = (m.transpose() * c * m)(0,0);
  	}
  
  	if (isinf(variance)) return false;
  	else return true;
}

// find the furthest point of a robot in a direction
Point AI::HL::STP::Evaluation::CMEvaluation::farthest(World &world, double time, int obs_flags, Point bbox_min,Point bbox_max, Point dir){
  	double x,max_x;
  	Point obs,max_obs;
  	double width;
  	int i;

  	max_x = 0;
  	max_obs = Point(0,0);

  	for(i=0; i<world.friendly_team().size(); i++){
    		if(obs_flags & OBS_TEAMMATE(i)){
      			width = Robot::MAX_RADIUS;
      			obs = world.friendly_team().get(i)->position(time);
      			if(inside_bbox(bbox_min,bbox_max,obs,width)){
        			x = obs.dot(dir) + width;

        			if(x > max_x){
          				max_x = x;
          				max_obs = obs;
        			}
      			}
    		}
  	}

  	width = OPPONENT_EFFECTIVE_RADIUS;

  	for(i=0; i<world.enemy_team().size(); i++){
    		if(obs_flags & OBS_OPPONENT(i)){
      			obs = world.enemy_team().get(i)->position(time);

      			if(inside_bbox(bbox_min,bbox_max,obs,width)){
        			x = obs.dot(dir) + width;

        			if(x > max_x){
          				max_x = x;
          				max_obs = obs;
        			}
      			}
    		}
  	}

  	max_obs = dir*max_x;
  	return(max_obs);
}

Point AI::HL::STP::Evaluation::CMEvaluation::findOpenPosition(World &world, Point p, Point toward, int obs_flags, double pradius){
  	obs_flags = obsLine(world, p, toward, obs_flags, TEAMMATE_EFFECTIVE_RADIUS, -1);
	
  	Point x = p;

  	while(1) {
    		if (!obsPosition(world, x, obs_flags, pradius)) break;
    
    		if ((toward - x).len() < TEAMMATE_EFFECTIVE_RADIUS) {
      			x = p; break; }

    		x += (toward - p).norm(TEAMMATE_EFFECTIVE_RADIUS);
  	}

  	return x;
}

Point AI::HL::STP::Evaluation::CMEvaluation::findOpenPositionAndYield(World &world, Point p, Point toward, int obs_flags){
  	p = findOpenPosition(world, p, toward, obs_flags, TEAMMATE_EFFECTIVE_RADIUS);
	
	// cm player type are either goalie or active
  	if (world.friendly_team().size() >= 1) p = findOpenPosition(world, p, toward, (obs_flags & OBS_TEAMMATE(world.friendly_team().size()-1)), 2 * TEAMMATE_EFFECTIVE_RADIUS);

  	return p;
}

