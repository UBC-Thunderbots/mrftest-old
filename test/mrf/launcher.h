#ifndef TEST_MRF_LAUNCHER_H
#define TEST_MRF_LAUNCHER_H

#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/window.h>
#include <memory>
#include "mrf/dongle.h"
#include "test/common/mapper.h"
#include "test/mrf/window.h"
#include "uicomponents/annunciator.h"

/**
 * \brief A launcher window from which testers for individual robots can be
 * launched.
 */
class TesterLauncher final : public Gtk::Window
{
   public:
    /**
     * \brief Constructs a new TesterLauncher.
     *
     * \param[in] dongle the radio dongle to use to communicate with robots.
     */
    explicit TesterLauncher(MRFDongle &dongle);

   private:
    MRFDongle &dongle;
    Gtk::VBox vbox;
    Gtk::Table table;
    Gtk::ToggleButton robot_toggles[8];
    std::unique_ptr<TesterWindow> windows[8];
    Gtk::ToggleButton mapper_toggle;
    std::unique_ptr<MapperWindow> mapper_window;
    GUIAnnunciator ann;

    void on_robot_toggled(unsigned int i);
    void on_mapper_toggled();
};

#endif
