#ifndef TESTER_DIRECT_DRIVE_H
#define TESTER_DIRECT_DRIVE_H

#include "test/permotor_drive.h"

class TesterControlDirectDrive : public TesterControlPerMotorDrive {
	public:
		TesterControlDirectDrive(XBeeDriveBot::Ptr bot) : TesterControlPerMotorDrive(bot) {
		}

		void drive(int m1, int m2, int m3, int m4) {
			robot->drive_direct(m1, m2, m3, m4);
		}
};

#endif

