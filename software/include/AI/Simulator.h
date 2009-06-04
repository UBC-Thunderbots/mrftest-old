#ifndef TB_SIMULATOR_H
#define TB_SIMULATOR_H

#include "datapool/DataSource.h"

class Simulator : public DataSource {
public:
	Simulator();
	void update();

private:
	Simulator(const Simulator &copyref); // Prohibit copying.
};

#endif

