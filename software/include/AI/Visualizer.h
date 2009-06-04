#ifndef TB_VISUALIZER_H
#define TB_VISUALIZER_H

#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>

class Visualizer : public Gtk::DrawingArea {
public:
	Visualizer();
	void update(void);

private:
	Gtk::Window win;
	bool on_expose_event(GdkEventExpose *event);
};

#endif
