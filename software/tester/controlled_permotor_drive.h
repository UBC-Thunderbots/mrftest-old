#ifndef TESTER_CONTROLLED_PERMOTOR_DRIVE_H
#define TESTER_CONTROLLED_PERMOTOR_DRIVE_H

#include "tester/permotor_drive.h"

class tester_control_controlled_permotor_drive : public tester_control_permotor_drive {
	public:
		tester_control_controlled_permotor_drive(xbee_drive_bot::ptr bot) : tester_control_permotor_drive(bot) {
		}

		void drive(int m1, int m2, int m3, int m4) {
			robot->drive_controlled(m1, m2, m3, m4);
		}
};

#endif

