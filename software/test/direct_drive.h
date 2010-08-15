#ifndef TESTER_DIRECT_DRIVE_H
#define TESTER_DIRECT_DRIVE_H

#include "test/permotor_drive.h"

/**
 * Allows driving the robot by choosing a power level for each of the four
 * wheels.
 */
class TesterControlDirectDrive : public TesterControlPerMotorDrive {
	public:
		/**
		 * Constructs a new TesterControlDirectDrive.
		 *
		 * \param[in] bot the robot to control.
		 */
		TesterControlDirectDrive(XBeeDriveBot::Ptr bot) : TesterControlPerMotorDrive(bot) {
		}

		void drive(int m1, int m2, int m3, int m4) {
			robot->drive_direct(m1, m2, m3, m4);
		}
};

#endif

