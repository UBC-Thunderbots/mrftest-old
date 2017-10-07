#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/textview.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include "ai/hl/hl.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/tactic/idle.h"
#include "util/dprint.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace
{
const Glib::ustring CHOOSE_PLAY_TEXT = u8"<Choose Play>";

// BoolParam use_gradient_pass(u8"Run pass calculation on seperate thread",
// u8"AI/HL/STP/PlayExecutor", true);

class STPHLChoosable final : public PlayExecutor, public HighLevel
{
   public:
    Gtk::VBox vbox;
    Gtk::Button stop_button;
    Gtk::Button start_button;
    Gtk::TextView text_status;
    Gtk::ComboBoxText combo;

    explicit STPHLChoosable(World world) : PlayExecutor(world)
    {
        combo.append(CHOOSE_PLAY_TEXT);
        for (const auto &i : Play::PlayFactory::all())
        {
            combo.append(i.second->name());
        }

        combo.set_active_text(CHOOSE_PLAY_TEXT);
        vbox.add(combo);
        vbox.add(start_button);
        vbox.add(stop_button);
        vbox.add(text_status);
        text_status.set_editable(false);
        start_button.set_label(u8"start");
        stop_button.set_label(u8"stop");

        start_button.signal_clicked().connect(
            sigc::bind(&STPHLChoosable::start, sigc::ref(*this)));
        stop_button.signal_clicked().connect(
            sigc::bind(&STPHLChoosable::stop, sigc::ref(*this)));
    }

    HighLevelFactory &factory() const;

    void start()
    {
        LOG_INFO(u8"start");

        // check if curr is valid
        if (curr_play)
        {
            return;
        }
        // check what play is in use
        if (combo.get_active_text() == CHOOSE_PLAY_TEXT)
        {
            curr_play = nullptr;
            return;
        }
        calc_play();
    }

    void stop()
    {
        LOG_INFO(u8"stop");

        curr_play = nullptr;
    }

    void calc_play() override
    {
        curr_play = nullptr;
        for (const auto &i : plays)
        {
            if (i->name() == combo.get_active_text())
            {
                curr_play = i->create(world);
            }
        }
        assert(curr_play);

        if (!curr_play->factory().invariant(world) || curr_play->done() ||
            curr_play->fail())
        {
            // only warn but still execute
            LOG_WARN(u8"Play not valid!");
        }
    }

    void tick() override
    {
        tick_eval(world);

        enable_players();

        // override halt completely
        if (world.friendly_team().size() == 0 ||
            world.playtype() == AI::Common::PlayType::HALT)
        {
            curr_play = nullptr;
        }

        // check what play is in use
        if (combo.get_active_text() == CHOOSE_PLAY_TEXT)
        {
            curr_play = nullptr;
        }

        if (curr_play &&
            combo.get_active_text() !=
                Glib::ustring(curr_play->factory().name()))
        {
            curr_play = nullptr;
        }

        /*
           if (curr_play && (!curr_play->invariant() || curr_play->done() ||
           curr_play->fail())) {
            LOG_INFO(u8"play done/no longer valid");
            curr_play = nullptr;
           }
         */

        Glib::ustring text;

        if (curr_play)
        {
            text = u8"Running";
            curr_play->tick(players_enabled);
        }
        else
        {
            text = u8"Stop";
        }

        if (curr_play)
        {
            text += info();
        }

        text_status.get_buffer()->set_text(text);
    }

    Gtk::Widget *ui_controls() override
    {
        return &vbox;
    }

    void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override
    {
        draw_ui(world, ctx);
        if (world.playtype() == AI::Common::PlayType::STOP)
        {
            ctx->set_source_rgb(1.0, 0.5, 0.5);
            ctx->arc(
                world.ball().position().x, world.ball().position().y, 0.5, 0.0,
                2 * M_PI);
            ctx->stroke();
        }
        if (!curr_play)
            return;
        curr_play->draw_overlay(ctx);
    }
};
}

HIGH_LEVEL_REGISTER(STPHLChoosable)
