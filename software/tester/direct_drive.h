#ifndef TESTER_DIRECT_DRIVE_H
#define TESTER_DIRECT_DRIVE_H

#include "tester/permotor_drive.h"

class tester_control_direct_drive : public tester_control_permotor_drive {
	public:
		tester_control_direct_drive(xbee_drive_bot::ptr bot) : tester_control_permotor_drive(bot) {
		}

		void drive(int m1, int m2, int m3, int m4) {
			robot->drive_direct(m1, m2, m3, m4);
		}
};

#endif

