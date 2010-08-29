#ifndef AI_WINDOW_H
#define AI_WINDOW_H

#include "ai/ai.h"
#include "uicomponents/visualizer.h"
#include <gtkmm.h>

namespace AI {
	/**
	 * A window for controlling the AI.
	 */
	class Window : public Gtk::Window {
		public:
			/**
			 * Creates a new main window.
			 *
			 * \param[in] ai the AI to observe and control.
			 *
			 * \param[in] vis \c true to start up with the visualizer visible, or \c
			 * false to start up with the visualizer invisible.
			 */
			Window(AI &ai, bool vis);

		private:
			AI &ai;
			Gtk::ComboBoxText playtype_override_chooser;
			Gtk::Entry playtype_entry;
			Gtk::ComboBoxText ball_filter_chooser;
			Gtk::Entry end_entry;
			Gtk::Entry refbox_colour_entry;
			Gtk::VBox coach_vbox;
			Gtk::ComboBoxText coach_chooser;
			Gtk::HBox strategy_hbox;
			Gtk::Label strategy_label;
			Gtk::Entry strategy_entry;
			Gtk::Widget *coach_controls;
			Gtk::VBox rc_vbox;
			Gtk::ComboBoxText rc_chooser;
			Gtk::Widget *rc_controls;
			Gtk::ToggleButton vis_button;
			Gtk::Window vis_window;
			Visualizer vis;

			void on_playtype_override_changed();
			void on_ball_filter_changed();
			void on_flip_ends_clicked();
			void on_flip_refbox_colour_clicked();
			void on_coach_changed();
			void on_rc_changed();
			void put_coach_controls();
			void on_playtype_changed();
			void on_vis_toggled();
			void on_flipped_ends();
			void on_flipped_refbox_colour();
			void on_strategy_changed(HL::Strategy::Ptr strat);
	};
}

#endif

