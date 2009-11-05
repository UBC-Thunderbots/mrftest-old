#ifndef SIMULATOR_FIELD_H
#define SIMULATOR_FIELD_H

#include "world/field.h"

//
// An implementation of "field" that uses the published theoretical dimensions
// of the field and is suitable for use in the simulator.
//
class simulator_field : public field {
	public:
		double length()               const { return 6.05; }
		double width()                const { return 4.05; }
		double goal_width()           const { return 0.7;  }
		double centre_circle_radius() const { return 0.5;  }
		double defense_area_radius()  const { return 0.5;  }
		double defense_area_stretch() const { return 0.35; }
};

#endif

