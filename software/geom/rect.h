#include "./geom/point.h"

class rect {
	public:
		/**
		Constructor taking the top left and bottom right corners
		\param point1 first point to define rectangle
		\param point2 diagonal opposite of point1
		*/
		rect(const point &point1, const point &point2);
	
		/**
		Constructor taking the top left and width with height
		\param top_left top left corner of the rectangle
		\param width width of the rectangle
		\param height height of the rectangle 
		*/
		rect(const point &sw_corner, double width, double height);

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
		point centre() const;
		
		/**
		\return the north east corner of the rectangle
		*/
		point ne_corner() const;

		/**
		\return the north west corner of the rectangle
		*/		
		point nw_corner() const;

		/**
		\return the south west corner of the rectangle
		*/
		point sw_corner() const;

		/**
		\return the south east corner of the rectangle
		*/
		point se_corner() const;		

		/**
		Translates a rectangle by the offset given by offset
		\param offset amount to move the rectangle
		*/
		void translate(const point &offset);		
		
		point min_corner;
		point diag;
};

