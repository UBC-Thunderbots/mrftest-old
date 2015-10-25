#ifndef DRIVE_NULL_DONGLE_H
#define DRIVE_NULL_DONGLE_H

#include "drive/dongle.h"
#include "drive/null/robot.h"

namespace Drive {
	namespace Null {
		class Dongle : public Drive::Dongle {
			public:
				Dongle();
				Drive::Robot &robot(unsigned int i);

			private:
				Robot bot;
		};
	}
}

#endif
