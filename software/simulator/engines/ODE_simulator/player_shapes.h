#include "geom/point.h"
#include <cmath>
#include <vector>
#include <ode/ode.h>

#warning this file needs Doxygen comments

namespace PlayerShape{

	const unsigned int CHOPPED_CIRCLE_APPROX = 15;
	const unsigned int NUM_POINTS = CHOPPED_CIRCLE_APPROX*2;//number of points that define this convex hull
	const unsigned int  PLANE_COUNT = CHOPPED_CIRCLE_APPROX + 2;

	dReal planes[PLANE_COUNT*4];
	dReal points[3*NUM_POINTS];
	unsigned int polygons[PLANE_COUNT + PLANE_COUNT*4 + 2*CHOPPED_CIRCLE_APPROX];
	const unsigned int POINT_COUNT = 30;

	std::vector<Point> get_chopped_circle(double radius, double face_width){

		double face_depth = sqrt(radius*radius - (face_width/2.0)*(face_width/2.0));
		std::vector<Point> ans;

		Point first(face_depth, face_width/2.0);
		Point last(face_depth, -face_width/2.0);

		double angle_face = acos(first.cross(last)/(radius*radius));
		double angle_step = (2*M_PI - angle_face)/(CHOPPED_CIRCLE_APPROX-1);

		for(unsigned int i=0; i<CHOPPED_CIRCLE_APPROX; i++){
			ans.push_back(first.rotate(i*angle_step));
		}

		return ans;
	
	}

	dGeomID get_player_geom(dSpaceID space, double height, double radius, double face_width){
		std::vector<Point> circle = get_chopped_circle(radius, face_width);
		//fill in the planes
		bool top= true;
	
		//fill in all of the planes
		for(unsigned int i=0; i < PLANE_COUNT; i++){
			if(i<circle.size()){//either the face or the round part of the robot	
				Point temp = circle[i] + circle[(i+1)%circle.size()];
				temp/=2;
				double len = temp.len();
				temp = temp.norm();
				planes[4*i]= temp.x;
				planes[4*i+1]= temp.y;
				planes[4*i+2]= 0.0;
				planes[4*i+3]= len;
			}else if(top){//the top part of the robot shape
				planes[4*i]= 0.0;
				planes[4*i+1]= 0.0;
				planes[4*i+2]= 1.0;
				planes[4*i+3]= height/2.0;
				top = false;
			}else{//the bottom part of the robot shape
				planes[4*i]= 0.0;
				planes[4*i+1]= 0.0;
				planes[4*i+2]= -1.0;
				planes[4*i+3]= height/2.0;
				top = true;
			}
		}
	
		//fill in the points
		for(unsigned int i=0; i<NUM_POINTS; i++){
			unsigned int circle_pt = i%circle.size();
			Point point_2d = circle[circle_pt];
			points[3*i] = point_2d.x;
			points[3*i+1] = point_2d.y;
			
			if(i<=circle_pt){//do top points first then bottom
				points[3*i+2] = height/2;
			}else{
				points[3*i+2] = -height/2;
			}
		}
	
		//fill in all the polygons
		int j=0;
		for(unsigned int i=0; i < PLANE_COUNT; i++){
			if(i<circle.size()){//either the face or the round part of the robot	
				polygons[j++]=4;
				polygons[j++]=i;
				polygons[j++]=i+circle.size();
				polygons[j++]=((i+1)%circle.size())+circle.size();
				polygons[j++]=(i+1)%circle.size();				
			}else if(top){//the top part of the robot shape
				polygons[j++]=circle.size();
				for(unsigned int k=0; k<circle.size(); k++){
					polygons[j++] = k;
				}
				top = false;
			}else{//the bottom part of the robot shape
				polygons[j++]=circle.size();
				for(int k=circle.size()-1; k>=0; k--){
					polygons[j++] = k + circle.size();
				}
				top = true;
			}
		}
		return dCreateConvex (space, planes, PLANE_COUNT, points, POINT_COUNT, polygons);
	}

}

