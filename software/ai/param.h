#ifndef AI_PARAM
#define AI_PARAM

#include "math.h"
#include "util/param.h"


//
//
// Temp hack to specify everything in degrees and get a return value in radians
//
//
//
//
class Temp_AngleParam : public DoubleParam {
 public:
	Temp_AngleParam(const char *name, const char *location, double def, double min, double max) : DoubleParam(name, location, def, min, max){
	}

	/**
	 * Returns the value of the parameter.
	 *
	 * \return the value of the parameter.
	 */
	operator double() const {
		return (adjustment()->get_value())*M_PI/180.0;
	}

};


namespace AI {
	/**
	 * Used in calculations of grab ball.
	 */
	extern DoubleParam player_average_velocity;

	extern Temp_AngleParam player_recieve_threshold;
}

#endif

