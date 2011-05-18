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

#include <stdio.h>
#include <cmath>

#include "ai/hl/world.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/cm_coordinate.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "geom/cm_util.h"

using AI::HL::STP::TCoordinate;
using AI::HL::STP::TRegion;
using namespace AI::HL::W;

Point TCoordinate::asVectorNotAbsolute(World &w){
  	Point v = c;
	/*
  	switch(side) {
  		case SBall:
    			v.y *= w.sideBall(); break;
	  	case SStrong:
	    		v.y *= w.sideStrong(); break;
	  	case SBallOrStrong:
	    		v.y *= w.sideBallOrStrong(); break;
	  	case SGui:
	    		v *= w.side; break;
	  	case SAbsolute:
	    	break;
  	}
        */
  	switch(origin) {
  		case OBall:
    			v += w.ball().position(); break;
  		case OAbsolute:
    			break;
  	}
  
  	if (!dynamic) {
    		c = v;
    		origin = OAbsolute;
    		side = SAbsolute;
    		absolute = true;
  	}
  
  	return v;
}

Point TRegion::center(World &w){
  	switch(type) {
  		case Rectangle: 
    			return (p[0].asVector(w) + p[1].asVector(w)) / 2.0;

  		case Circle: 
  		default:
    			return p[0].asVector(w);
  	}
}

Point TRegion::sample(World &w){
  	switch(type) {
  		case Rectangle: {
    			Point v0 = p[0].asVector(w);
    			Point v1 = p[1].asVector(w);
    			double w = (std::rand() / RAND_MAX * 2 * radius) - radius;
    			double l = std::rand() / RAND_MAX * (v0 - v1).len();

    			return v0 + (v1 - v0).norm(l) + (v1 - v0).perp().norm(w);
  		}

  		case Circle: 
  		default: {
    			double r = sqrt(std::rand() / RAND_MAX) * radius;
    			double a = std::rand() / RAND_MAX * 2 * M_PI;

    			return p[0].asVector(w) + Point(r, 0).rotate(a);
  		}

	}
}

Point TRegion::centerVelocity(World &w){
  	switch(type) {
  		case Rectangle:
    			return (p[0].getVelocity(w) + p[1].getVelocity(w)) / 2.0;

  		case Circle: 
  		default:
    			return p[0].getVelocity(w);
  	}
}

void TRegion::diagonal(World &w, Point x, Point &d1, Point &d2){
  	switch(type) {
  		case Rectangle: {
    			Point v0 = p[0].asVector(w);
    			Point v1 = p[1].asVector(w);
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

  		case Circle:
  		default: {
    			Point center = p[0].asVector(w);
    			d1 = center + (x - center).perp().norm(radius);
    			d2 = center - (x - center).perp().norm(radius);
    			break;
  		}
  	}

}

bool TRegion::inRegion(World &w, Point x){
  	switch(type) {
  		case Rectangle: {
    			Point v0 = p[0].asVector(w);
    			Point v1 = p[1].asVector(w);

    			return ((v0 - v1).dot(x - v1) > 0 && (v1 - v0).dot(x - v0) > 0 && std::fabs(distance_to_line(v0, v1, x)) < radius);

  		}
  		case Circle: 
  		default:
    		return (x - center(w)).len() < radius;
  	}
}


