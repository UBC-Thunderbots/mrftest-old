#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "util/dprint.h"
#include <cmath>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/textview.h>

using AI::HL::HighLevelFactory;
using AI::HL::HighLevel;
using namespace AI::HL::STP;

namespace {
	class STPHL : public PlayExecutor, public HighLevel {
		public:
			Gtk::VBox vbox;
			Gtk::Button reset_button;
			Gtk::TextView text_view;

			STPHL(World &world) : PlayExecutor(world) {
				text_view.set_editable(false);
				vbox.add(reset_button);
				vbox.add(text_view);
				reset_button.set_label("reset");
				reset_button.signal_clicked().connect(sigc::bind(&STPHL::reset, sigc::ref(*this)));
			}

			void reset() {
				curr_play = 0;
			}

			HighLevelFactory &factory() const;

			void tick() {
				PlayExecutor::tick();
				text_view.get_buffer()->set_text(info());
			}

			Gtk::Widget *ui_controls() {
				return &vbox;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				PlayExecutor::draw_overlay(ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(STPHL)

