#ifndef TESTER_CONTROLLED_PERMOTOR_DRIVE_H
#define TESTER_CONTROLLED_PERMOTOR_DRIVE_H

#include "test/permotor_drive.h"

class TesterControlControlledPerMotorDrive : public TesterControlPerMotorDrive {
	public:
		TesterControlControlledPerMotorDrive(XBeeDriveBot::ptr bot) : TesterControlPerMotorDrive(bot) {
		}

		void drive(int m1, int m2, int m3, int m4) {
			robot->drive_controlled(m1, m2, m3, m4);
		}
};

#endif

