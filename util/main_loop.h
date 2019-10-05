#ifndef UTIL_MAIN_LOOP_H
#define UTIL_MAIN_LOOP_H

namespace Gtk
{
class Window;
}

namespace MainLoop
{
/**
 * \brief Runs the main loop until \ref quit() is called.
 */
void run();

/**
 * \brief Runs the main loop until \ref quit() is called or \p window is hidden.
 *
 * \param[in] window the window to display
 */
void run(Gtk::Window &window);

/**
 * \brief Exits the main loop.
 */
void quit();

/**
 * \brief Exits the main loop, propagating the currently-caught exception.
 *
 * This function must be invoked from inside a catch block.
 */
void quit_with_current_exception();
}

#endif
