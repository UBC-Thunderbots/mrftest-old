#ifndef UICOMPONENTS_ANNUNCIATOR_H
#define UICOMPONENTS_ANNUNCIATOR_H

#include <gtkmm/cellrenderertext.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>

/**
 * A graphical panel that displays annunciator messages.
 */
class GUIAnnunciator final : public Gtk::ScrolledWindow
{
   public:
    /**
     * \brief Constructs a new annunciator panel ready to add to a window.
     */
    explicit GUIAnnunciator();

   private:
    Gtk::TreeView view;
    Gtk::CellRendererText message_renderer;
};

#endif
