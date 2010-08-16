#include "./geom/point.h"

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
		 * Translates the rectangle.
		 *
		 * \param[in] offset the distance to move the rectangle.
		 */
		void translate(const Point &offset);		
		
	private:
		Point min_corner;
		Point diag;
};

