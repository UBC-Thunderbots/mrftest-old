#ifndef GEOM_RECT_H
#define GEOM_RECT_H

#include "geom/point.h"

/**
 * A rectangle.
 */
class Rect {
	public:
		/**
		 * Creates a new Rect from two corners.
		 *
		 * \param[in] point1 one of the rectangle's corners.
		 *
		 * \param[in] point2 the corner diagonally-opposite to \p point1.
		 */
		Rect(const Point &point1, const Point &point2);

		/**
		 * Creates a new Rect from a corner and a size.
		 *
		 * \param[in] sw_corner the south-west corner of the rectangle.
		 *
		 * \param[in] width the width of the rectangle.
		 *
		 * \param[in] height the height of the rectangle.
		 */
		Rect(const Point &sw_corner, double width, double height);

		/**
		 * Returns the width of the rectangle.
		 *
		 * \return the width of the rectangle.
		 */
		double width() const;

		/**
		 * Returns the height of the rectangle.
		 *
		 * \return the height of the rectangle.
		 */
		double height() const;

		/**
		 * Returns the area of the rectangle.
		 *
		 * \return the area of the rectangle.
		 */
		double area() const;

		/**
		 * Returns the centre of the rectangle.
		 *
		 * \return the centre of the rectangle.
		 */
		Point centre() const;

		/**
		 * Returns the north-east corner of the rectangle.
		 *
		 * \return the north-east corner of the rectangle.
		 */
		Point ne_corner() const;

		/**
		 * Returns the north-west corner of the rectangle.
		 *
		 * \return the north-west corner of the rectangle.
		 */
		Point nw_corner() const;

		/**
		 * Returns the south-west corner of the rectangle.
		 *
		 * \return the south-west corner of the rectangle.
		 */
		Point sw_corner() const;

		/**
		 * Returns the south-east corner of the rectangle.
		 *
		 * \return the south-east corner of the rectangle.
		 */
		Point se_corner() const;

		/**
		 * Returns 0-sw 1-nw 2-ne 3-se corner for pos%4
		 *
		 * \param[in] pos of corner wanted
		 *
		 * \return Point corresponding to position
		 */
		Point operator[](unsigned int pos);

		/**
		 * Translates the rectangle.
		 *
		 * \param[in] offset the distance to move the rectangle.
		 */
		void translate(const Point &offset);

		/**
		 * Checks whether a point is within the boundries of the rectangle
		 *
		 * \param[in] Point to test
		 *
		 * \return bool whether the point is inside the boundry of the rectangle
		 */
		bool point_inside(Point p);

		/**
		 * Tries to move all of the edges of the rectangle outwards/inwards towards the centre by "amount"
		 * while keeping the location of the centre of the rectangle the same the rectangle 
		 * will not shrink to something smaller than a point
		 *
		 * \param[in] the amount to shrink the recatngle by on all sides (positive or negative numbers ok)
		 *
		 * \return bool whether it was possible to expand/shrink the rectangle by amount requested
		 */
		bool expand(double amount);

		/**
		 * Gives the distance between a point and the nearest point on the rectangle boundry
		 *
		 * \param[in] Point to test
		 *
		 * \return double
		 */
		double dist_to_boundary(Point p);

	private:
		Point min_corner;
		Point diag;
};

#endif

