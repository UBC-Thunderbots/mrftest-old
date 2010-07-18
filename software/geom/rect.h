#include "./geom/point.h"

class Rect {
	public:
		/**
		Constructor taking the top left and bottom right corners
		\param point1 first point to define rectangle
		\param point2 diagonal opposite of point1
		*/
		Rect(const Point &point1, const Point &point2);
	
		/**
		Constructor taking the top left and width with height
		\param top_left top left corner of the rectangle
		\param width width of the rectangle
		\param height height of the rectangle 
		*/
		Rect(const Point &sw_corner, double width, double height);

		/**
		\return width of rectangle
		*/
		double width() const;
		
		/**
		\return height of rectangle
		*/
		double height() const;
	
		/**
		\return area of the rectangle
		*/	
		double area() const;
		
		/**
		\return the centre of the rectangle
		*/
		Point centre() const;
		
		/**
		\return the north east corner of the rectangle
		*/
		Point ne_corner() const;

		/**
		\return the north west corner of the rectangle
		*/		
		Point nw_corner() const;

		/**
		\return the south west corner of the rectangle
		*/
		Point sw_corner() const;

		/**
		\return the south east corner of the rectangle
		*/
		Point se_corner() const;		

		/**
		Translates a rectangle by the offset given by offset
		\param offset amount to move the rectangle
		*/
		void translate(const Point &offset);		
		
		Point min_corner;
		Point diag;
};

