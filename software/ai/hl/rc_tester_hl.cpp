#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/scale.h>

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	struct RCTester : public HighLevel {
		World world;
		Gtk::VBox vbox;
		Gtk::Button reset_button;
		Gtk::HScale controls[3];

		RCTester(World world) : world(world) {
			for (Gtk::HScale &i : controls) {
				vbox.add(i);
				// params are
				// min, max, step, intervals
				i.get_adjustment()->configure(0, -5, 5, 0.1, 100, 0);
				i.set_digits(2);
			}
			vbox.add(reset_button);
			reset_button.set_label(u8"reset");
			reset_button.signal_clicked().connect(sigc::bind(&RCTester::reset, sigc::ref(*this)));
		}

		void reset() {
			for (Gtk::HScale &i : controls) {
				i.set_value(0);
			}
		}

		void tick() {
			FriendlyTeam friendly = world.friendly_team();
			if (friendly.size() < 1) {
				LOG_INFO(u8"error: must have at least one robot on the field!");
				return;
			}

			for (Player runner : friendly) {
				const double px = runner.position().x + controls[0].get_value();
				const double py = runner.position().y + controls[1].get_value();
				const Angle pz = runner.orientation() + Angle::of_radians(controls[2].get_value());

				runner.move(Point(px, py), pz, Point());
				runner.type(AI::Flags::MoveType::NORMAL);
			}
		}

		Gtk::Widget *ui_controls() {
			return &vbox;
		}

		HighLevelFactory &factory() const;
	};
}

HIGH_LEVEL_REGISTER(RCTester)

