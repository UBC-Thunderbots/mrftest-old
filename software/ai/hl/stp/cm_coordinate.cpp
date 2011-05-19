// tactic.cc
// 
// Parent class for tactics.
//
// Created by:  Michael Bowling (mhb@cs.cmu.edu)
//
/* LICENSE:
  =========================================================================
    CMDragons'02 RoboCup F180 Source Code Release
  -------------------------------------------------------------------------
    Copyright (C) 2002 Manuela Veloso, Brett Browning, Mike Bowling,
                       James Bruce; {mmv, brettb, mhb, jbruce}@cs.cmu.edu
    School of Computer Science, Carnegie Mellon University
  -------------------------------------------------------------------------
    This software is distributed under the GNU General Public License,
    version 2.  If you do not have a copy of this licence, visit
    www.gnu.org, or write: Free Software Foundation, 59 Temple Place,
    Suite 330 Boston, MA 02111-1307 USA.  This program is distributed
    in the hope that it will be useful, but WITHOUT ANY WARRANTY,
    including MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  ------------------------------------------------------------------------- */

#include "ai/hl/stp/cm_coordinate.h"
#include "ai/hl/util.h"
#include "ai/hl/world.h"
#include "ai/hl/stp/evaluation/cm_evaluation.h"
#include "geom/angle.h"
#include "geom/util.h"
#include <cmath>
#include <cstdio>

using AI::HL::STP::TCoordinate;
using AI::HL::STP::TRegion;
using namespace AI::HL::W;

namespace Evaluation = AI::HL::STP::Evaluation;

Point TCoordinate::as_vector_not_absolute(World &w){
  	Point v = c;
	
  	switch(side) {
  		case SType::BALL:
    			v.y *= Evaluation::side_ball(w); 
			break;
	  	case SType::STRONG:
	    		v.y *= Evaluation::side_strong(w); 
			break;
	  	case SType::BALL_OR_STRONG:
	    		v.y *= Evaluation::side_ball_or_strong(w); 
			break;
	  	case SType::ABSOLUTE:
	    	break;
  	}
        
  	switch(origin) {
		case OType::BALL:
    			v += w.ball().position(); 
			break;
		case OType::ABSOLUTE:
    			break;
  	}
  
  	if (!dynamic) {
    		c = v;
    		origin = OType::ABSOLUTE;
    		side = SType::ABSOLUTE;
    		absolute = true;
  	}
  
  	return v;
}

Point TRegion::center(World &w){
  	switch(type) {
  		case Type::RECTANGLE: 
    			return (p[0].as_vector(w) + p[1].as_vector(w)) / 2.0;

  		case Type::CIRCLE: 
    			return p[0].as_vector(w);
  	}
}

Point TRegion::sample(World &w){
  	switch(type) {
  		case Type::RECTANGLE: {
    			Point v0 = p[0].as_vector(w);
    			Point v1 = p[1].as_vector(w);
    			double w = (std::rand() / static_cast<double>(RAND_MAX) * 2 * radius) - radius;
    			double l = std::rand() / static_cast<double>(RAND_MAX) * (v0 - v1).len();

    			return v0 + (v1 - v0).norm(l) + (v1 - v0).perp().norm(w);
  		}

  		case Type::CIRCLE: {
    			double r = std::sqrt(std::rand() / static_cast<double>(RAND_MAX)) * radius;
    			double a = std::rand() / static_cast<double>(RAND_MAX) * 2 * M_PI;

    			return p[0].as_vector(w) + Point(r, 0).rotate(a);
  		}

	}
}

Point TRegion::center_velocity(World &w){
  	switch(type) {
  		case Type::RECTANGLE:
    			return (p[0].get_velocity(w) + p[1].get_velocity(w)) / 2.0;

  		case Type::CIRCLE: 
    			return p[0].get_velocity(w);
  	}
}

void TRegion::diagonal(World &w, Point x, Point &d1, Point &d2){
  	switch(type) {
  		case Type::RECTANGLE: {
    			Point v0 = p[0].as_vector(w);
    			Point v1 = p[1].as_vector(w);
    			Point c = center(w);
    			Point r[4] = {
      				v0 - (v0 - v1).perp().norm(radius),
      				v0 + (v0 - v1).perp().norm(radius),
      				v1 + (v0 - v1).perp().norm(radius),
      				v1 - (v0 - v1).perp().norm(radius) };

    			Point perp = (x - c).perp();

    			d1 = segment_near_line(r[0], r[1], c, c + perp);
    			if ((d1.x == r[0].x && d1.y == r[0].y) || (d1.x == r[1].x && d1.y == r[1].y))
      				d1 = segment_near_line(r[1], r[2], c, c + perp);

    			d2 = segment_near_line(r[2], r[3], c, c - perp);
    			if ((d2.x == r[2].x && d2.y == r[2].y) || (d2.x == r[3].x && d2.y == r[3].y))
      				d2 = segment_near_line(r[3], r[0], c, c - perp);

    			break;
  		}

  		case Type::CIRCLE:
  		{
    			Point center = p[0].as_vector(w);
    			d1 = center + (x - center).perp().norm(radius);
    			d2 = center - (x - center).perp().norm(radius);
    			break;
  		}
  	}

}

bool TRegion::in_region(World &w, Point x){
  	switch(type) {
  		case Type::RECTANGLE: {
    			Point v0 = p[0].as_vector(w);
    			Point v1 = p[1].as_vector(w);

    			return ((v0 - v1).dot(x - v1) > 0 && (v1 - v0).dot(x - v0) > 0 && std::fabs(line_point_dist(x, v0, v1)) < radius);

  		}
  		case Type::CIRCLE: 
    		return (x - center(w)).len() < radius;
  	}
}

