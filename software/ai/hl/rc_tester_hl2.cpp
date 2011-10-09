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
	struct RCTesterFactory : public HighLevelFactory {
		RCTesterFactory() : HighLevelFactory("RC Tester 2") {
		}

		HighLevel::Ptr create_high_level(World &world) const;
	};

	RCTesterFactory factory_instance;

	struct RCTester : public HighLevel {
		World &world;
		Gtk::VBox vbox;
		Gtk::HScale controls[3];
		Gtk::HScale offsets_x;
		Gtk::HScale offsets_y;
		Gtk::Button reset_button;

		RCTester(World &world) : world(world) {
			for (int i = 0; i < 3; ++i) {
				vbox.add(controls[i]);
				// params are
				// min, max, step, intervals
				controls[i].get_adjustment()->configure(0, -5, 5, 0.1, 100, 0);
				controls[i].set_digits(2);
			}

			controls[0].get_adjustment()->configure(0, -3, 3, 0.1, 100, 0);
			controls[1].get_adjustment()->configure(0, -2, 2, 0.1, 100, 0);

			vbox.add(offsets_x);
			vbox.add(offsets_y);

			offsets_x.get_adjustment()->configure(Robot::MAX_RADIUS * 4, -2, 2, 0.1, 100, 0);
			offsets_x.set_digits(2);

			offsets_y.get_adjustment()->configure(0, -2, 2, 0.1, 100, 0);
			offsets_y.set_digits(2);

			vbox.add(reset_button);
			reset_button.set_label("reset");
			reset_button.signal_clicked().connect(sigc::bind(&RCTester::reset, sigc::ref(*this)));
		}

		void reset() {
			for (int i = 0; i < 3; ++i) {
				controls[i].set_value(0);
			}
			offsets_x.set_value(0);
			offsets_y.set_value(0);
		}

		void tick() {
			FriendlyTeam &friendly = world.friendly_team();
			if (friendly.size() < 1) {
				LOG_INFO("error: must have at least one robot on the field!");
				return;
			}

			const double px = controls[0].get_value();
			const double py = controls[1].get_value();
			const Angle pz = Angle::of_radians(controls[2].get_value());

			for (std::size_t i = 0; i < friendly.size(); ++i) {
				Player::Ptr runner = friendly.get(i);

				const double ix = px + offsets_x.get_value() * static_cast<double>(i);
				const double iy = py + offsets_y.get_value() * static_cast<double>(i);

				runner->move(Point(ix, iy), pz, Point());
				runner->type(AI::Flags::MoveType::NORMAL);
			}
		}

		Gtk::Widget *ui_controls() {
			return &vbox;
		}

		RCTesterFactory &factory() const {
			return factory_instance;
		}
	};

	HighLevel::Ptr RCTesterFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new RCTester(world));
		return p;
	}
}

