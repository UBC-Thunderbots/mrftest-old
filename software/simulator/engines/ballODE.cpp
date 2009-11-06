#include "simulator/engines/ballODE.h"

/*

ballODE.h has the following:

public:



private:



*/

			
			ballODE::ballODE(dWorldID dworld) : the_position(0.0, 0.0), the_velocity(0.0, 0.0) {

world = dworld;
			}



			point ballODE::position() const {
				return the_position;
			}

			void ballODE::ui_set_position(const point &pos) {
			}


