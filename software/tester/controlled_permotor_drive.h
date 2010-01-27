#ifndef TESTER_CONTROLLED_PERMOTOR_DRIVE_H
#define TESTER_CONTROLLED_PERMOTOR_DRIVE_H

#include "tester/permotor_drive.h"

class tester_control_controlled_permotor_drive : public tester_control_permotor_drive {
	public:
		void drive(int16_t m1, int16_t m2, int16_t m3, int16_t m4) {
			robot->drive_controlled(m1, m2, m3, m4);
		}
};

#endif

