#ifndef PHYSICS_H
#define PHYSICS_H


//This file contains all the physical constants of the robot
//Dimensions and the like as well as 

#define CONTROL_LOOP_HZ 200U
#define QUARTERDEGREE_TO_MS (0.0000554f * CONTROL_LOOP_HZ)
#define QUARTERDEGREE_TO_RPM (CONTROL_LOOP_HZ / 240.0f) //encoder quarter of degree to motor RPM
#define RPM_TO_VOLT (1.0f/374.0f) //motor RPM to back EMF

#define HALL_PHASE_TO_MS (0.00171*CONTROL_LOOP_HZ)

#define QUARTERDEGREE_TO_VOLT QUARTERDEGREE_TO_RPM*RPM_TO_VOLT

#define ROBOT_RADIUS 0.085f
#define TICK_TIME (1.0f / CONTROL_LOOP_HZ)
#define ROBOT_POINT_MASS 2.48f
#define DELTA_VOLTAGE_LIMIT 4.25f  //Voltage where wheel slips (acceleration cap)

//all the interial components of the robot
//This one is a little strange as it is the effective rotational mass
//The rotational mass * (ROBOT_RADIUS)^2 will give the conventional interia
#define INERTIAL_FACTOR 0.37f

//factor for steel motor mounts
#define STEEL_INTERTIAL_FACTOR 0.3858f

#define ROT_MASS INERTIAL_FACTOR*ROBOT_POINT_MASS
#define INERTIA ROT_MASS*ROBOT_RADIUS*ROBOT_RADIUS

#define CURRENT_PER_TORQUE 39.21f //from motor data sheet (1/25.5 mNm)
#define PHASE_RESISTANCE 1.6f //adjust this number as calculated
#define GEAR_RATIO 0.5143f //define as speed multiplication from motor to wheel
#define WHEEL_RADIUS 0.0254f

#define MAX_X_V 2.0f //maximal linear velocity in the X direction
#define MAX_Y_V 1.0f //maximum linear velocity in the Y direction
#define MAX_T_V 0.1f //max robot rotation rate in radians per second

extern const float MAX_VEL[3];

// WRONG NUMBERS, POKE JON TO GET ACTUAL NUMBERS
#define MAX_X_A 3.0f
#define MAX_Y_A 3.0f
#define MAX_T_A 30.0f

extern const float MAX_ACC[3];

//gyro running at 2000/second and in integers such that 32767 is 2000
//61.0 millidegrees/second / LSB
#define DEGREES_PER_GYRO (61.0f/1000.0f)
#define MS_PER_DEGREE (2.0f*(float)M_PI*ROBOT_RADIUS/360.0f)
#define MS_PER_GYRO MS_PER_DEGREE*DEGREES_PER_GYRO

extern const float ROBOT_MASS[3];
extern const float MAX_VEL[3];

//transformation matricies to convert speeds in the
//two different domains commonly used by the robot
//speed4 which is the listing of wheel speeds 
//and speed3 which is a speed in x,y,rotation in 
//robot relative coordinates
void speed4_to_speed3(const float speed4[4], float speed3[3]);
void speed3_to_speed4(const float speed3[3], float speed4[4]);

//rotate a velocity vector through angle
void rotate(float speed3[2], float angle);


//returns the amount to scale accel by to hit
//the acceleraton limits for the robot
//acceleration should be in robot local coordinates
// vector is accelerations in 
// {m/s^2 m/s^2 rad/s^2}
float get_maximal_accel_scaling(const float linear_accel[2], float angular_accel);

//returns the amount to scale wheel torque by to hit the voltage limits
//takes a vector of motor torques in (Nm)
float get_maximal_torque_scaling(const float torque[4]);

//vector subtraction
void vectorSub(float *a,const float *b, int len);

//polar cartesian transforms
void Cart2Pol(float vec[2]);
void Pol2Cart(float vec[2]);
void CartVel2Pol(float const loc[2], float vel[2]);
void PolVel2Cart(float const loc[2], float vel[2]);
void PolAcc2Cart(float const loc[2], float const vel[2], float const Pacc[2], float Cacc[2]);

void matrix_mult(float* lhs, int lhs_len, const float* rhs,int rhs_len, const float matrix[lhs_len][rhs_len]);

#endif