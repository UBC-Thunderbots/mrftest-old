#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/legacy_tactic.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "ai/hl/stp/action/legacy_move.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "geom/util.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/play/play.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
using namespace AI::HL::STP::Play::Global;
using AI::HL::STP::Coordinate;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;


namespace {
	class BlockShotPath final : public LegacyTactic {
		public:
	  explicit BlockShotPath(World world, unsigned int index, double max_dist) : LegacyTactic(world), index(index), max_dist_from_net(max_dist) {
			}

		private:
			std::vector<Point> destinations;
			Player select(const std::set<Player> &players) const override;
			unsigned int index;
			double max_dist_from_net;
			void execute() override;
			Glib::ustring description() const override {
				return u8"block secondary shot";
			}
	};

	Player BlockShotPath::select(const std::set<Player> &players) const {
	  Point dest;
	  if(destinations.size() > index){
	    dest = destinations[index];
	  }else {
	    dest = world.field().friendly_goal();
	  }
	       return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest));
	}

	void BlockShotPath::execute() {
	        double future_block_time = 0.7;
		std::vector<Point> obstacles;
		std::vector<std::pair<Player, const AI::HL::STP::Tactic::Tactic*>> tactic_assignments = get_tactic_assignment();
		for (auto i : tactic_assignments) {
			if (dynamic_cast<const BlockShotPath*>(i.second)) {

			}
			else {
			    obstacles.push_back(i.first.position());
			}
		}
	   
		std::vector<Robot> enemy_threats = AI::HL::STP::Evaluation::rank_enemy_passee_risk(world, obstacles, future_block_time);

		Point enemy_to_block;
		if (enemy_threats.size() > index) {
			enemy_to_block = enemy_threats[index] ? enemy_threats[index].position(future_block_time) : Point();
		}

		bool goalie_top = true;
		const Field &field = world.field();
		const Point goal_side = goalie_top ? Point(-field.length() / 2, field.goal_width() / 2) : Point(-field.length() / 2, -field.goal_width() / 2);
		const Point goal_opp = goalie_top ? Point(-field.length() / 2, -field.goal_width() / 2) : Point(-field.length() / 2, field.goal_width() / 2);

	      

		std::pair<Point, Angle> risk_zone = angle_sweep_circles(enemy_to_block, goal_side, goal_opp, obstacles , Robot::MAX_RADIUS);
		//Angle ang = risk_zone.second * 1;

		
		//Point c1 = (risk_zone.first - enemy_to_block).rotate(ang/2) + enemy_to_block;
		//Point c2 = (risk_zone.first - enemy_to_block).rotate(-ang/2) + enemy_to_block;
		Point destination = enemy_to_block + 0.35*(risk_zone.first - enemy_to_block).norm();

		Point goal_intersect = line_intersect(enemy_to_block, risk_zone.first, goal_opp, goal_side);

		if ((enemy_to_block - goal_intersect).len() > max_dist_from_net){
		  
		  destination = goal_intersect + max_dist_from_net*(enemy_to_block - goal_intersect).norm();

		  //line_intersect(enemy_to_block, risk_zone.first, Point(-world.field().width()*1.0/8.0,10.0),Point(-world.field().width()*1.0/8.0, -10.0));
		}
		//Point destination = calc_block_cone(c1, c2, enemy_to_block, Robot::MAX_RADIUS);
		if (destinations.size() > index){
		  destinations[index] = destination;
		}else{
		  destinations.push_back(destination);
		}
		Action::move(world, player, destination);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::block_shot_path(World world, unsigned int index, double max_dist) {
  Tactic::Ptr p(new BlockShotPath(world, index, max_dist));
	return p;
}
