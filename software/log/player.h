#ifndef LOG_PLAYER_H
#define LOG_PLAYER_H

#include "uicomponents/visualizer.h"
#include <memory>
#include <string>
#include <gtkmm/window.h>

class LogPlayer final : public Gtk::Window {
	public:
		/**
		 * \brief Creates a new LogPlayer.
		 *
		 * \param[in] parent the parent window.
		 *
		 * \param[in] pathname the path to the log file to play.
		 */
		explicit LogPlayer(Gtk::Window &parent, const std::string &pathname);

	private:
		class Impl;

		std::unique_ptr<Impl> impl;

		bool on_delete_event(GdkEventAny *);
};

#endif

