#ifndef AI_WINDOW_H
#define AI_WINDOW_H

#include "ai/ai.h"
#include "ai/strategy/strategy.h"
#include "robot_controller/robot_controller.h"
#include "ai/world/world.h"
#include <gtkmm.h>

/**
 * A window for controlling the AI.
 */
class ai_window : public Gtk::Window {
	public:
		/**
		 * Creates a new main window.
		 */
		ai_window(ai &ai);

	private:
		ai &the_ai;
		Gtk::Entry playtype_entry;
		Gtk::VBox strategy_vbox;
		Gtk::ComboBoxText strategy_chooser;
		Gtk::Widget *strategy_controls;
		Gtk::VBox rc_vbox;
		Gtk::ComboBoxText rc_chooser;
		Gtk::Widget *rc_controls;

		void on_strategy_changed();
		void on_rc_changed();
		void put_strategy_controls();
		void on_playtype_changed();
};

#endif

