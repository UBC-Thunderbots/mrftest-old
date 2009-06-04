#ifndef TB_EMERGENCYSTOPBUTTON_H
#define TB_EMERGENCYSTOPBUTTON_H

namespace EmergencyStopButton {
	void init();
	void update();
	extern bool state; // FALSE = run, TRUE = kill
}

#endif

