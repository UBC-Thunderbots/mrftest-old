#include "ai/hl/stp/tactic/tri_attack_active.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/util.h"
#include "geom/util.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;
using AI::HL::STP::Coordinate;

namespace {
	
	class tri_attack_active : public Tactic {
		public:
			tri_attack_active(World world, unsigned i) : Tactic(world), index(i) {
			}

		private:
			unsigned index;
			Player select(const std::set<Player> &players) const;
			void execute();
			Glib::ustring description() const {
				return "tri_attack_active";
			}
	};

//	Player tri_attack_active::select(const std::set<Player> &players) const {
//		Point target = Evaluation::evaluate_tdefense(world, index);
//		return Player;/**std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(target)) */
//	}

	void tri_attack_active::execute() {

		}
//		Action::tri_attack_active(world, player, target);
	}
//void tri_attack_active::execute(world, i) {
//do something
//}


