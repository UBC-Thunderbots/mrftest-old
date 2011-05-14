#ifndef AI_HL_STP_REGION_H
#define AI_HL_STP_REGION_H

#include "geom/rect.h"
#include "ai/hl/stp/coordinate.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * Describes a region, either a circle or a rectangle.
			 * The region can make use of dynamic coordinates.
			 * See STP paper section 5.2.3 (b)
			 */
			class Region {
				public:
					enum Type {
						RECTANGLE,
						CIRCLE,
					};

					/**
					 * RTTI
					 * i.e. returns the type of this region.
					 */
					Type type() const {
						return type_;
					}

					/**
					 * Obtain the (calculated) center of this region.
					 */
					virtual Point center() const = 0;

					/**
					 * Checks if a point in inside this region.
					 */
					virtual bool inside(Point p) const = 0;

				protected:
					Region(Type t) : type_(t) {
					}

				private:
					Type type_;
			};

			/**
			 * Describes a rectangular region.
			 */
			class Rectangle : public Region {
				public:
					Rectangle(Coordinate p1, Coordinate p2) : Region(RECTANGLE), p1(p1), p2(p2) {
					}

					Point center() const;

				private:
					Coordinate p1, p2;
			};

			/**
			 * Describes a circular region.
			 */
			class Circle : public Region {
				public:
					Circle(Coordinate c, double r) : Region(CIRCLE), center_(c), radius_(r) {
					}

					Point center() const {
						return center_();
					}

					double radius() const {
						return radius_;
					}

				private:
					Coordinate center_;
					double radius_;
			};
		}
	}
}

#endif

