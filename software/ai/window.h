#ifndef AI_WINDOW_H
#define AI_WINDOW_H

#include "ai/ai.h"
#include "ai/strategy/strategy.h"
#include "ai/world/world.h"
#include "robot_controller/robot_controller.h"
#include "uicomponents/visualizer.h"
#include <gtkmm.h>

/**
 * A window for controlling the AI.
 */
class ai_window : public Gtk::Window {
	public:
		/**
		 * Creates a new main window.
		 *
		 * \param[in] ai the AI to observe and control.
		 *
		 * \param[in] vis \c true to start up with the visualizer visible, or \c
		 * false to start up with the visualizer invisible.
		 */
		ai_window(ai &ai, bool vis);

	private:
		ai &the_ai;
		Gtk::ComboBoxText playtype_override_chooser;
		Gtk::Entry playtype_entry;
		Gtk::ComboBoxText ball_filter_chooser;
		Gtk::Entry end_entry;
		Gtk::Entry refbox_colour_entry;
		Gtk::VBox strategy_vbox;
		Gtk::ComboBoxText strategy_chooser;
		Gtk::Widget *strategy_controls;
		Gtk::VBox rc_vbox;
		Gtk::ComboBoxText rc_chooser;
		Gtk::Widget *rc_controls;
		Gtk::ToggleButton vis_button;
		Gtk::Window vis_window;
		visualizer vis;

		void on_playtype_override_changed();
		void on_ball_filter_changed();
		void on_flip_ends_clicked();
		void on_flip_refbox_colour_clicked();
		void on_strategy_changed();
		void on_rc_changed();
		void put_strategy_controls();
		void on_playtype_changed();
		void on_vis_toggled();
		void on_flipped_ends();
		void on_flipped_refbox_colour();
};

#endif

