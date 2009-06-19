#ifndef UI_CONTROLPANEL_H
#define UI_CONTROLPANEL_H

#include "datapool/Noncopyable.h"

#include <memory>

class ControlPanelImpl;
class ControlPanel : private virtual Noncopyable {
public:
	ControlPanel();

private:
	std::auto_ptr<ControlPanelImpl> impl;
};

#endif

