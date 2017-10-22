#include "control.h"
#include "simulate.h"
//#include "dribbler.h"
//#include "leds.h"
#include "primitive.h"
#include "physics.h"
#include "bangbang.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// these are set to decouple the 3 axis from each other
// the idea is to clamp the maximum velocity and acceleration
// so that the axes would never have to compete for resources
#define TIME_HORIZON 0.05f //s
#define NUM_PATHPOINTS 300 
#define MAX_MAJOR_ACCEL 1.9 
#define MAX_MAJOR_VEL 2.5 

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


typedef struct{
	float x;
	float y;
}Vec;

Vec sub(Vec a, Vec b){
	Vec ret;
	ret.x = a.x - b.x;
	ret.y = a.y - b.y;
	return ret;
}

Vec add(Vec a, Vec b){
	Vec ret;
	ret.x = a.x + b.x;
	ret.y = a.y + b.y;
	return ret;
}

float dot(Vec a, Vec b){
	float ret = a.x*b.x + a.y*b.y;
	return ret;
}

float cross(Vec a, Vec b){
	float ret = a.x*b.y - a.y*b.x;
	return ret;
}
float mag(Vec a){
	float ret = sqrtf(a.x*a.x + a.y*a.y);
	return ret;
}

Vec norm(Vec a){
	Vec ret;
	float m = mag(a);
	ret.x = a.x/m; 
	ret.y = a.y/m; 
	return ret;	
}

Vec mult(Vec a, float scale){
	Vec ret;
	ret.x = a.x*scale;	
	ret.y = a.y*scale;	
	return ret;
}

Vec rotateVec(Vec a, float angle) {
	Vec ret;
	ret.x = cosf(angle)*a.x - sinf(angle)*a.y;
    ret.y = sinf(angle)*a.x + cosf(angle)*a.y;
	return ret;
}

static inline float splineWt(int ptNum, float t){
	switch(ptNum){
		case 0:
			return (1-t)*(1-t)*(1-t);
		case 1:
			return 3*t*(1-t)*(1-t);
		case 2:
			return 3*t*t*(1-t);
		case 3:
			return t*t*t;
		default:
			return 0.0;
	}
}

float minorAccel(Vec seg1, Vec seg2, float majorVel){
	if(majorVel <= 0) return 0.0;
	float seg1len = mag(seg1);
	float seg2len = mag(seg2);
	return 2.0*cross(seg1,seg2)*majorVel*majorVel/(mag(add(seg1,seg2))*seg1len*seg2len);
}
float maxMajorVel(Vec seg1, Vec seg2){
	if(dot(seg1,seg2) <= 0 ) return 0.0;
	float seg1len = mag(seg1);
	float seg2len = mag(seg2);

	float maxMinorAccel = 0.5;
	return sqrt(maxMinorAccel*mag(add(seg1,seg2))*seg1len*seg2len/(2.0*fabs(cross(seg1,seg2))));
}



static Vec major_vec, minor_vec;
float distAfterCurSeg;
Vec path[NUM_PATHPOINTS];
unsigned nextTarget; // which point in the path to aim for

void updateLineSeg(const Vec seg){
	float length = mag(seg);
	distAfterCurSeg -= length; // reduce the distance by the length of this segment
	major_vec.x = seg.x/length; 
	major_vec.y = seg.y/length;
	minor_vec = rotateVec(major_vec, M_PI/2);	
	//printf("\n\nmajor vec: <%f,%f>", major_vec.x, major_vec.y);
	//printf("\nminor vec: <%f,%f>\n\n", minor_vec.x, minor_vec.y);
	//printf("\nNext target y: %f", p1y);
}

/**
 * \brief Initializes the spline primitive.
 *
 * This function runs once at system startup.
 */
static void spline_init(void) 
{
	// Currently has nothing to do
}

/**
 * \brief Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a spline
 * movement.
 *
 * \param[in] params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 */
void spline_start(const primitive_params_t *params) 
{
  
	// Convert into m/s and rad/s because physics is in m and s
	dr_data_t state;
	dr_get(&state);

	// initialize the four control points
	Vec ctrl[4];
	ctrl[0].x = state.x;
	ctrl[0].y = state.y;
	ctrl[1].x = state.x + state.vx; // this should probably be passed in as a param
	ctrl[1].y = state.y + state.vy; // this should probably be passed in as a param
	ctrl[2].x = ((float)(params->params[0])/1000.0f);
	ctrl[2].y = ((float)(params->params[1])/1000.0f);
	ctrl[3].x = ((float)(params->params[2])/1000.0f);
	ctrl[3].y = ((float)(params->params[3])/1000.0f);
	
	for(unsigned i=0;i<NUM_PATHPOINTS;i++){
		float t = (float)i/(NUM_PATHPOINTS-1.0);
		path[i].x = splineWt(0,t)*ctrl[0].x + splineWt(1,t)*ctrl[1].x + splineWt(2,t)*ctrl[2].x + splineWt(3,t)*ctrl[3].x;
		path[i].y = splineWt(0,t)*ctrl[0].y + splineWt(1,t)*ctrl[1].y + splineWt(2,t)*ctrl[2].y + splineWt(3,t)*ctrl[3].y;
	}

	distAfterCurSeg = 0;
	for(unsigned i=1;i<NUM_PATHPOINTS;i++){
		distAfterCurSeg += mag(sub(path[i], path[i-1]));	
	}

	nextTarget = 1; // aim for the second point in the path (not the first because we are already there)
	updateLineSeg(sub(path[1],path[0])); // update the local line segment to follow	
}

/**
 * \brief Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * spline movement is already in progress.
 */
static void spline_end(void) 
{
}

/**
 * \brief Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */
void spline_tick() {
	dr_data_t state;
	dr_get(&state);
	
	Vec pos;
	pos.x = state.x;
	pos.y = state.y;

	Vec vel;
	vel.x = state.vx;
	vel.y = state.vy;

	Vec relPos = sub(pos, path[nextTarget]);
	
	float major_disp = dot(relPos, major_vec); 
	float minor_disp = dot(relPos, minor_vec); 
	//printf("\nminor disp: %f", minor_disp);

	// check if we should move onto the next segment of the path
	while((major_disp > - 0.002) && nextTarget < (NUM_PATHPOINTS - 1)){
		
		nextTarget++; // move onto the next segment of the path
		updateLineSeg(sub(path[nextTarget],path[nextTarget-1])); // update the local line segment	
		
		// recalculate major/minor displacement
		Vec relPos = sub(pos, path[nextTarget]);
		major_disp = dot(relPos, major_vec); 
		minor_disp = dot(relPos, minor_vec); 
		//printf("\nmajor disp: %f", major_disp);
	}

	//determine the end speed for this current segment
	float major_vel = dot(major_vec, vel);

	float dist = -major_disp;
	float endVel;
	float major_accel = 9.0;
	for(unsigned index=nextTarget;index<NUM_PATHPOINTS;index++){
		if(index == NUM_PATHPOINTS -1){
			endVel = 0.0;
		}else{
			endVel = maxMajorVel(sub(path[index], path[index-1]), sub(path[index+1],path[index]));	
		}
		
		BBProfile major_profile;
		PrepareBBTrajectoryMaxV(&major_profile, dist, major_vel, endVel, MAX_MAJOR_ACCEL,MAX_MAJOR_VEL); //3.5, 3.0
		PlanBBTrajectory(&major_profile);
		float test_accel = BBComputeAvgAccel(&major_profile, TIME_HORIZON); // perhaps should reduce major accel if minor displacement is large
		major_accel = min(test_accel, major_accel);
		if(index > nextTarget){
			dist += mag(sub(path[index],path[index-1]));
		}

	}

	float minor_vel = dot(minor_vec, vel);
	float minor_accel = 25.0*(-minor_disp- 0.5*minor_vel); // add position and vel control
	if(nextTarget < (NUM_PATHPOINTS - 1)){
		minor_accel += minorAccel(sub(path[nextTarget],path[nextTarget-1]),sub(path[nextTarget+1],path[nextTarget]), major_vel);	
	}
	float accel[3] = {0};

	float local_x_norm_vec[2] = {cosf(state.angle), sinf(state.angle)}; 
	float local_y_norm_vec[2] = {cosf(state.angle + M_PI/2.0), sinf(state.angle + M_PI/2.0)}; 
	
	//project accel into robot reference frame
	accel[0] = minor_accel*(local_x_norm_vec[0]*minor_vec.x + local_x_norm_vec[1]*minor_vec.y );
	accel[0] += major_accel*(local_x_norm_vec[0]*major_vec.x + local_x_norm_vec[1]*major_vec.y );
	accel[1] = minor_accel*(local_y_norm_vec[0]*minor_vec.x + local_y_norm_vec[1]*minor_vec.y );
	accel[1] += major_accel*(local_y_norm_vec[0]*major_vec.x + local_y_norm_vec[1]*major_vec.y );

	apply_accel(accel, 0); // accel is already in local coords
} 
