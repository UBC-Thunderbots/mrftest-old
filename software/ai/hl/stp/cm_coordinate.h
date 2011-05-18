// tactic.h
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

#ifndef AI_HL_STP_CM_COORDINATE_H
#define AI_HL_STP_CM_COORDINATE_H

#include "ai/hl/world.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "geom/cm_util.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Coordinate
			 */
			class TCoordinate {
				public:
			  		enum otype { OAbsolute, OBall };
			  		enum stype { SAbsolute, SBall, SStrong, SBallOrStrong};

				private:
			  		otype origin;
			  		stype side;
			  		bool dynamic;
			  		bool absolute;

			  		Point c;

			  		Point as_vector_not_absolute(AI::HL::W::World &w);
			  
				public:
			  		TCoordinate(double x, double y, stype _side = SAbsolute, otype _origin = OAbsolute, bool _dynamic = false) {
						c = Point(x, y);
			    			side = _side; origin = _origin; dynamic = _dynamic; 
			    			absolute = (origin == OAbsolute && side == SAbsolute); 
					}

			  		TCoordinate(Point _p = Point(0, 0), stype _side = SAbsolute, otype _origin = OAbsolute, bool _dynamic = false) {
			    			c = _p;
			    			side = _side; origin = _origin; dynamic = _dynamic; 
			    			absolute = (origin == OAbsolute && side == SAbsolute); 
					}

			  		Point as_vector(AI::HL::W::World &w) {
			    			if (absolute) return c;
			    			else return as_vector_not_absolute(w);
			  		}

			  		double as_direction(AI::HL::W::World &w) {
			    			return as_vector(w).orientation(); 
					}

			  		Point get_velocity(AI::HL::W::World &w) {
			    			if (dynamic && origin == OBall) return w.ball().velocity();
			    			else return Point(0, 0); 
					}
			};


			/**
			 * A Region can be either a circle (can also be a point, which is a circle with 0 radius) or a rectangle
			 */
			class TRegion {
				private:
			  		enum { Circle,  Rectangle } type;
			  
			  		TCoordinate p[2];
			  
			  		double radius;
			  
				public:
			  		TRegion() { 
						type = Circle; 
						radius = 0; // point
					} 
			  		TRegion(TCoordinate p1, TCoordinate p2, double _radius) {
			    			type = Rectangle; 
						p[0] = p1; 
						p[1] = p2; 
						radius = _radius; 
					}
			  		TRegion(TCoordinate p1, double _radius) {
			    			type = Circle; 
						p[0] = p1; 
						radius = _radius; 
					}
					
					/**
			 		 * returns the center of the region
			 		 */
			  		Point center(AI::HL::W::World &w);

					/**
			 		 * returns a random sample point in the region
			 		 */
			  		Point sample(AI::HL::W::World &w); 

			  		Point center_velocity(AI::HL::W::World &w);

			  		void diagonal(AI::HL::W::World &w, Point p, Point &d1, Point &d2);

					/**
			 		 * checks if Point p is in region
			 		 */
			  		bool in_region(AI::HL::W::World &w, Point p);

			};		
		}
	}
}
#endif
