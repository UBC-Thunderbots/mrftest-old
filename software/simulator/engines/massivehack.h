#include "robot_controller/robot_controller.h"


class massivehack {
public:
void getRequestedVelocities(robot_controller::ptr controller, point &velocity, double &angular_velocity);

};



void massivehack::getRequestedVelocities(robot_controller::ptr controller, point &velocity, double &angular_velocity){



if(controller){
const point temp;
double a;
controller->move(temp, temp, a, a, velocity, angular_velocity);




	double tempp = 	velocity.x;
	velocity.x = -velocity.y;
	velocity.y = tempp;

//velocity =  velocity/50.00;	
//double t = angular_velocity/10.00;

}

}

