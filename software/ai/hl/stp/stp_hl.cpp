#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/textview.h>
#include <cmath>
#include "ai/hl/hl.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/util.h"
#include "util/dprint.h"

using AI::HL::HighLevelFactory;
using AI::HL::HighLevel;
using namespace AI::HL::STP;

namespace
{
class STPHL final : public PlayExecutor, public HighLevel
{
   public:
    Gtk::VBox vbox;
    Gtk::Button reset_button;
    Gtk::TextView text_view;

    explicit STPHL(World world) : PlayExecutor(world)
    {
        text_view.set_editable(false);
        vbox.add(reset_button);
        vbox.add(text_view);
        reset_button.set_label(u8"reset");
        reset_button.signal_clicked().connect(
            sigc::bind(&STPHL::reset, sigc::ref(*this)));
    }

    void reset()
    {
        curr_play = nullptr;
    }

    HighLevelFactory &factory() const override;

    void tick() override
    {
        PlayExecutor::tick();
        const Glib::ustring &i = info();
        text_view.get_buffer()->set_text(i);
        ai_notes = i;
    }

    Gtk::Widget *ui_controls() override
    {
        return &vbox;
    }

    void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override
    {
        PlayExecutor::draw_overlay(ctx);
    }
};
}

HIGH_LEVEL_REGISTER(STPHL)
