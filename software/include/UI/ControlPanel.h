#ifndef TB_CONTROLPANEL_H
#define TB_CONTROLPANEL_H

#include <tr1/memory>
#include <gtkmm/table.h>

class ControlPanelImpl;

class ControlPanel : public Gtk::Table {
public:
	ControlPanel();
	void update();

private:
	std::tr1::shared_ptr<ControlPanelImpl> impl;
};

#endif

