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

#include "ai/hl/util.h"
#include "ai/hl/world.h"
#include "geom/angle.h"
#include "geom/util.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Coordinate
			 */
			class TCoordinate {
				public:
			  		enum class OType {
						ABSOLUTE,
						BALL,
					};

			  		enum class SType {
						ABSOLUTE,
						BALL,
						STRONG,
						BALL_OR_STRONG,
					};
			  
			  		TCoordinate(double x, double y, SType side_ = SType::ABSOLUTE, OType origin_ = OType::ABSOLUTE, bool dynamic_ = false);

			  		TCoordinate(Point p = Point(), SType side_ = SType::ABSOLUTE, OType origin_ = OType::ABSOLUTE, bool dynamic_ = false);

			  		Point as_vector(const AI::HL::W::World &w);

			  		double as_direction(const AI::HL::W::World &w);

			  		Point get_velocity(const AI::HL::W::World &w);

				private:
			  		Point c;
			  		SType side;
			  		OType origin;
			  		bool dynamic;
			  		bool absolute;

			  		Point as_vector_not_absolute(const AI::HL::W::World &w);
			};


			/**
			 * A Region can be either a circle (can also be a point, which is a circle with 0 radius) or a rectangle
			 */
			class TRegion {
				public:
			  		TRegion();

			  		TRegion(const TCoordinate &p1, const TCoordinate &p2, double radius);

			  		TRegion(const TCoordinate &p1, double radius);
					
					/**
			 		 * returns the center of the region
			 		 */
			  		Point center(const AI::HL::W::World &w);

					/**
			 		 * returns a random sample point in the region
			 		 */
			  		Point sample(const AI::HL::W::World &w); 

			  		Point center_velocity(const AI::HL::W::World &w);

			  		void diagonal(const AI::HL::W::World &w, Point p, Point &d1, Point &d2);

					/**
			 		 * checks if Point p is in region
			 		 */
			  		bool in_region(const AI::HL::W::World &w, Point p);

				private:
			  		enum class Type {
						CIRCLE,
						RECTANGLE,
					};
					
					Type type;
			  
			  		TCoordinate p[2];
			  
			  		double radius;
			};
		}
	}
}

inline AI::HL::STP::TCoordinate::TCoordinate(double x, double y, SType side_, OType origin_, bool dynamic_) : c(x, y), side(side_), origin(origin_), dynamic(dynamic_), absolute(origin == OType::ABSOLUTE && side == SType::ABSOLUTE) {
}

inline AI::HL::STP::TCoordinate::TCoordinate(Point p, SType side_, OType origin_, bool dynamic_) : c(p), side(side_), origin(origin_), dynamic(dynamic_), absolute(origin == OType::ABSOLUTE && side == SType::ABSOLUTE) {
}

inline Point AI::HL::STP::TCoordinate::as_vector(const AI::HL::W::World &w) {
	if (absolute) {
		return c;
	} else {
		return as_vector_not_absolute(w);
	}
}

inline double AI::HL::STP::TCoordinate::as_direction(const AI::HL::W::World &w) {
	return as_vector(w).orientation(); 
}

inline Point AI::HL::STP::TCoordinate::get_velocity(const AI::HL::W::World &w) {
	if (dynamic && origin == OType::BALL) {
		return w.ball().velocity();
	} else {
		return Point();
	}
}

inline AI::HL::STP::TRegion::TRegion() : type(Type::CIRCLE), radius(0) {
	// This is actually a point.
}

inline AI::HL::STP::TRegion::TRegion(const TCoordinate &p1, const TCoordinate &p2, double radius) : type(Type::RECTANGLE), radius(radius) {
	p[0] = p1;
	p[1] = p2;
}

inline AI::HL::STP::TRegion::TRegion(const TCoordinate &p1, double radius) : type(Type::CIRCLE), radius(radius) {
	p[0] = p1;
}

#endif

