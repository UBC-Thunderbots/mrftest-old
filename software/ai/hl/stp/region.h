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
					 * You can use this information to downcast to subclass.
					 */
					Type type() const {
						return type_;
					}

					/**
					 * Evaluates the position of the center of this region.
					 */
					virtual Point center_position() const = 0;

					/**
					 * Evaluates the velocity of the center of this region.
					 */
					virtual Point center_velocity() const = 0;

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

					Point center_position() const {
						return (p1.position() + p2.position()) / 2;
					}

					Point center_velocity() const {
						return (p1.velocity() + p2.velocity()) / 2;
					}

					/**
					 * Evaluate the rectangle associated.
					 */
					Rect evaluate() const {
						return Rect(p1.position(), p2.position());
					}

					bool inside(Point p) const {
						return Rect(p1.position(), p2.position()).point_inside(p);
					}

				private:
					Coordinate p1, p2;
			};

			/**
			 * Describes a circular region.
			 */
			class Circle : public Region {
				public:
					Circle(Coordinate c, double r) : Region(CIRCLE), center(c), radius_(r) {
					}

					/**
					 * Evaluates the center of this circle.
					 */
					Point center_position() const {
						return center.position();
					}

					Point center_velocity() const {
						return center.velocity();
					}

					double radius() const {
						return radius_;
					}

					bool inside(Point p) const {
						return (p - center.position()).len() <= radius_;
					}

				private:
					Coordinate center;
					double radius_;
			};
		}
	}
}

#endif

