#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/scale.h>
#include <gtkmm/comboboxtext.h>

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	struct RCTester2 final : public HighLevel {
		World world;
		Gtk::VBox vbox;
		Gtk::HScale controls[3];
		Gtk::HScale offsets_x;
		Gtk::HScale offsets_y;
		Gtk::Button reset_button;
		Gtk::ComboBoxText mp_choose;

		explicit RCTester2(World world) : world(world) {
			for (Gtk::HScale &i : controls) {
				vbox.add(i);
				// params are
				// min, max, step, intervals
				i.get_adjustment()->configure(0, -5, 5, 0.1, 100, 0);
				i.set_digits(2);
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
			reset_button.set_label(u8"reset");
			reset_button.signal_clicked().connect(sigc::bind(&RCTester2::reset, sigc::ref(*this)));
		
			mp_choose.append(u8"movement = move");
			mp_choose.append(u8"movement = dribble");
			mp_choose.append(u8"movement = shoot");
			mp_choose.append(u8"movement = spin");

			vbox.add(mp_choose);
		}

		void reset() {
			for (Gtk::HScale &i : controls) {
				i.set_value(0);
			}
			offsets_x.set_value(0);
			offsets_y.set_value(0);
		}

		void tick() override {
			FriendlyTeam friendly = world.friendly_team();
			if (friendly.size() < 1) {
				LOG_INFO(u8"error: must have at least one robot on the field!");
				return;
			}

			const double px = controls[0].get_value();
			const double py = controls[1].get_value();
			const Angle pz = Angle::of_radians(controls[2].get_value());

			int row = mp_choose.get_active_row_number();

			for (std::size_t i = 0; i < friendly.size(); ++i) {
				Player runner = friendly[i];
				
				const double ix = px + offsets_x.get_value() * static_cast<double>(i);
				const double iy = py + offsets_y.get_value() * static_cast<double>(i);
				switch( row ){
					case 0: 
						runner.mp_move(Point(ix, iy), pz);
						break;
					case 1: 
						runner.mp_dribble(Point(ix, iy), pz);
						break;
					case 2: 
						runner.mp_shoot(Point(ix, iy), pz, false, 4.0);
						break;
					case 3: 
						runner.mp_spin(Point(ix, iy), pz);
						break;
				}
				runner.type(AI::Flags::MoveType::NORMAL);
			}
		}

		Gtk::Widget *ui_controls() override {
			return &vbox;
		}

		HighLevelFactory &factory() const override;
	};
}

HIGH_LEVEL_REGISTER(RCTester2)

