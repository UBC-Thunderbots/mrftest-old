#ifndef UI_CONTROLPANEL_H
#define UI_CONTROLPANEL_H

#include "datapool/Noncopyable.h"

class ControlPanelImpl;
class ControlPanel : private virtual Noncopyable {
public:
	ControlPanel();
	~ControlPanel();

private:
	ControlPanelImpl *impl;
};

#endif

