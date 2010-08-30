#ifndef TESTER_CONTROLLED_PERMOTOR_DRIVE_H
#define TESTER_CONTROLLED_PERMOTOR_DRIVE_H

#include "test/permotor_drive.h"

/**
 * Allows driving the robot by choosing a speed for each of the four wheels and having the on-board firmware run a control loop to maintain that speed.
 */
class TesterControlControlledPerMotorDrive : public TesterControlPerMotorDrive {
	public:
		/**
		 * Constructs a new TesterControlControlledPerMotorDrive.
		 *
		 * \param[in] bot the robot to control.
		 */
		TesterControlControlledPerMotorDrive(XBeeDriveBot::Ptr bot) : TesterControlPerMotorDrive(bot) {
		}

		void drive(int m1, int m2, int m3, int m4) {
			robot->drive_controlled(m1, m2, m3, m4);
		}
};

#endif

