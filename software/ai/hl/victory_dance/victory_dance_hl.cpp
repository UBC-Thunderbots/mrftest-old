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
	struct VDHL : public HighLevel {
		
		World &world;
		Gtk::VBox vbox;
		Gtk::Button reset_button;
		Gtk::HScale dest_control;

		VDHL(World &world) : world(world) {
			vbox.add(dest_control);
			// params are
			// min, max, step, intervals
			dest_control.get_adjustment()->configure(0, -1.5, 1.5, 0.1, 30, 0);
			dest_control.set_digits(2);
		
			vbox.add(reset_button);
			reset_button.set_label("reset");
			reset_button.signal_clicked().connect(sigc::bind(&VDHL::reset, sigc::ref(*this)));
		}
		
		HighLevelFactory &factory() const;
		
		void reset() {
			dest_control.set_value(0);
		}
		
		void tick() {
			FriendlyTeam &friendly = world.friendly_team();

			for (uint robotIndex = 0; robotIndex < friendly.size(); robotIndex++) {
				Point des(dest_control.get_value(), 0);
				double radius = 0.2 * robotIndex + 0.2;
				Angle offset_angle = Angle::of_radians(0.7 + robotIndex * 1.1);
				Player::Ptr runner = friendly.get(robotIndex);
				Point diff = (des - friendly.get(0)->position()).rotate(offset_angle);
				Point dest = des - radius * (diff / diff.len());

				runner->flags(0);
				runner->type(AI::Flags::MoveType::NORMAL);
				runner->prio(AI::Flags::MovePrio::HIGH);
				runner->move(dest, (des - runner->position()).orientation().angle_mod(), Point());
			}
		}

		Gtk::Widget *ui_controls() {
			return &vbox;
		}
			
	};
}

HIGH_LEVEL_REGISTER(VDHL)

