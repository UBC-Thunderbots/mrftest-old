#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "geom/util.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/play/play.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
using namespace AI::HL::STP::Play::Global;
using AI::HL::STP::Coordinate;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;


namespace {
	class BlockShotPath final : public Tactic {
		public:
	  explicit BlockShotPath(World world, unsigned int index, double max_dist) : Tactic(world), index(index), max_dist_from_net(max_dist) {
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> context) const override {
				context->arc(enemy_to_block.x, enemy_to_block.y, 0.3, 0, 3.14159 * 2);
				context->stroke();
			}

		private:
			std::vector<Point> destinations;
			Player select(const std::set<Player> &players) const override;
			unsigned int index;
	   	//Seung-new define variable
			Point original_pos;
			Point enemy_to_block;
			//Point test;
			double max_dist_from_net;

			void execute(caller_t& ca) override;

			Glib::ustring description() const override {
				return u8"block shot path";
			}
	};

	//Define Class player
	//Compare the distance from the 'dest' and return the the player who has the shortest distance
	Player BlockShotPath::select(const std::set<Player> &players) const {
	  Point dest;
	  if(destinations.size() > index){
	    dest = destinations[index];
	  }else {
	    dest = world.field().friendly_goal();
	  }
	       return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest));
	}

	void BlockShotPath::execute(caller_t& ca) {
		while (true) {
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

			// Enemy_threats ranks the enemy robots that are closer to the enemy baller.
			std::vector<Robot> enemy_threats = AI::HL::STP::Evaluation::rank_enemy_passee_risk(world, obstacles, future_block_time);

			// Enemy_theats.size() is always 5.
			// If enemy_threats.size is smaller than index. all enemy_to_ is just becomeing the point (0.0)
			if (enemy_threats.size() > index) {
				enemy_to_block = enemy_threats[index] ? enemy_threats[index].position(future_block_time) : Point();
			}


			const Field &field = world.field();
			//negative field represents ourside of the field.
			const Point goal_side = Point(-field.length() / 2, -field.goal_width() / 2);
			const Point goal_opp  = Point(-field.length() / 2, field.goal_width() / 2);



			std::pair<Point, Angle> risk_zone = angle_sweep_circles(enemy_to_block, goal_side, goal_opp, obstacles , Robot::MAX_RADIUS);
			// Angle ang = risk_zone.second * 1;

			Point destination = enemy_to_block + 0.35*(risk_zone.first - enemy_to_block).norm();

			Point goal_intersect = line_intersect(enemy_to_block, risk_zone.first, goal_opp, goal_side);

			if ((enemy_to_block - goal_intersect).len() > max_dist_from_net){

			  destination = goal_intersect + max_dist_from_net*(enemy_to_block - goal_intersect).norm();

			  //line_intersect(enemy_to_block, risk_zone.first, Point(-world.field().width()*1.0/8.0,10.0),Point(-world.field().width()*1.0/8.0, -10.0));
			}
			//Point destination = calc_block_cone(c1, c2, enemy_to_block, Robot::MAX_RADIUS);
			if (destinations.size() > index){
			  destinations[index] = destination;
			}
			else{
			  destinations.push_back(destination);
			}

			//original_pos = player().position();
			player().move_move(destination, (destination - player().position()).orientation());

			yield(ca);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::block_shot_path(World world, unsigned int index, double max_dist) {
  Tactic::Ptr p(new BlockShotPath(world, index, max_dist));
	return p;
}
