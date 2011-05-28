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
			 *
			 * NOTE: class needs to be COPYABLE,
			 * hence it is not subclassed.
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
					Point center_position() const;

					/**
					 * Evaluates the velocity of the center of this region.
					 */
					Point center_velocity() const;

					/**
					 * Valid only if this region type is a circle.
					 * Returns the associated radius.
					 */
					double radius() const {
						return radius_;
					}

					/**
					 * Valid only if this region type is a rectangle.
					 * Evaluates the associated rectangle.
					 */
					Rect rectangle() const {
						return Rect(p1.position(), p2.position());
					}

					/**
					 * Checks if a point in inside this region.
					 */
					bool inside(Point p) const;

					/**
					 * Create a circle region.
					 */
					static Region circle(Coordinate center, double radius);

					/**
					 * Create a rectangle region.
					 */
					static Region rectangle(Coordinate p1, Coordinate p2);

					/**
					 * Copy constructor.
					 */
					Region(const Region &region);

				protected:
					Type type_;
					Coordinate p1, p2;
					double radius_;

					/**
					 * The actual constructor, hidden from public view.
					 */
					Region(Type type, Coordinate p1, Coordinate p2, double radius);
			};
		}
	}
}

#endif

