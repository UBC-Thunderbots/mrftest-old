#ifndef TB_XBEE_H
#define TB_XBEE_H

#include "datapool/Team.h"

namespace XBee {
	//
	// The format of an outbound packet from host to robot.
	//
	struct TXData {
		// Desired left-right velocity.
		char vx;

		// Desired forward-reverse velocity.
		char vy;

		// Desired rotational velocity.
		char vtheta;

		// Power level to dribbler.
		unsigned char dribble;

		// Power level to kicker.
		unsigned char kick;

		// Emergency stop control (0=run, nonzero=kill).
		unsigned char emergency;

		// Vision-measured left-right velocity (-128=not available).
		char vxMeasured;

		// Vision-measured forward-reverse velocity (-128=not available).
		char vyMeasured;

		// Extra controls (-128=not available).
		char extra1;
		char extra2;
	} __attribute__((packed));

	//
	// The format of an inbound packet from robot to host.
	//
	struct RXData {
		// Green battery voltage (0-1023), high and low order bits (65536 if no data yet).
		unsigned char vGreen[2];

		// Motor battery voltage (0-1023), high and low order bits (65536 if no data yet).
		unsigned char vMotor[2];
	} __attribute__((packed));

	//
	// Initializes the XBee subsystem.
	//
	void init();

	//
	// Transmits and receives pending data.
	//
	void update();

	//
	// Communication status indicators for the robots.
	//
	enum CommStatus {
		// Packets are not even being acknowledged.
		STATUS_FAILED,
		// Packets are being acknowledged but feedback is not being received.
		STATUS_NO_RECV,
		// Feedback data is being received.
		STATUS_OK
	};

	//
	// Whether or not the last packet sent to this robot was acknowledged.
	//
	extern CommStatus commStatus[Team::SIZE];

	//
	// The data to send to the robots.
	//
	extern TXData out[Team::SIZE];

	//
	// The data most recently received from the robots.
	//
	extern RXData in[Team::SIZE];
}

#endif

