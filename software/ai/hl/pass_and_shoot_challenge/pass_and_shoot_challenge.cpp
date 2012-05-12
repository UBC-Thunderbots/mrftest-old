#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/flags.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/intercept.h"
#include "geom/util.h"
#include<iostream>

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	const unsigned int min_team_size = 4;

	Point bot0_initial(-2.75,-1.2);
	Point bot1_initial(-2.00,1.2);
	Point bot2_initial(-1.00,-1.2);
	Point bot3_initial(0.00,1.2);
	Angle robot0_orientation = (bot1_initial - bot0_initial).orientation();
	Angle robot1_orientation = (bot0_initial - bot1_initial).orientation();
	Angle robot1_orientation_dos= (bot2_initial - bot1_initial).orientation();
	Angle robot2_orientation = (bot1_initial - bot2_initial).orientation();
	Angle robot3_orientation = (bot2_initial - bot3_initial).orientation();
	double kick_speed = 0.05;

	enum state{
		/*
		 * Set up bots to initial positions. Bot0 = (-2.5, -1.2) Bot1 = (-2.00, 1.2) Bot2 = (-1.00, -1.2) Bot3 = (0.00, 1.2)
			Returns to this state if things go wrong
		*/
		INITIAL_POSITION,
		/*
		 * Bot0 waits until it has possesion of ball. it then passes ball to Bot1.
		 * Bot1 moves along horizontally along y=1.2 and intercepts ball.
		 */
		BOT0_PASS,
		/*
		 * Bot1 rotates and faces Bot2.
		 * if Bot1 has possesion of ball, pass to Bot2
		 */
		BOT1_PASS,
		/*
		 * Bot2 rotates and faces Bot3
		 * if Bot2 has posession of ball, pass to Bot3
		 * on next_state, Bot0 goes to position WRITE_NEW_POSITION_HERE
		 */
		BOT2_PASS,
		/*
		 * Bot3 rotates to face bot0
		 * if bot3 has possesion of ball, pass to bot0
		 * on next state, bot1 goes to WRITE_NEW_POSITION_HERE
		 */
		BOT3_PASS,
		/*
		 * bot0 rotates to face bot1
		 * if b0t0 has possesion of ball, pass to bot1
		 * on next state, bot2 goes to WRITE_NEW_POSITION_HERE
		 */
		BOT0_REPOS,
		/*
		 * bot1 rotates to face bot2
		 * if bot1 still has possesion of ball, pass to bot2
		 * on next state, bot3 goes to WRITE_NEW_POSITION HERE
		 */
		BOT1_REPOS,
		/*
		 * bot2 rotates to face bot3
		 * if bot2 still has possesion of ball, pass to bot3
		 */
		BOT2_REPOS,
		/*
		 * If bot3 still has ball, rotate until facing center of goal
		 * shoot and score.
		 */
		BOT3_REPOS};

};


class PASCHL : public HighLevel {
		public:
			PASCHL(World &world) : world(world) {
				robot_positions.push_back(std::make_pair(bot0_initial, robot0_orientation));
				robot_positions.push_back(std::make_pair(bot1_initial,robot1_orientation));
				robot_positions.push_back(std::make_pair(bot2_initial,robot2_orientation));
				robot_positions.push_back(std::make_pair(bot3_initial,robot3_orientation));

				current_state = INITIAL_POSITION;

				kicked_ball = false;
			}

			HighLevelFactory &factory() const;

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();

				if(friendly.size() < min_team_size) {
					return;
				}

				Player::Ptr player0 = friendly.get(0);
				Player::Ptr player1 = friendly.get(1);
				Player::Ptr player2 = friendly.get(2);
				//Player::Ptr player3 = friendly.get(3);

				switch(current_state) {
				case INITIAL_POSITION:
					for(unsigned int i = 0 ;i < min_team_size; i++) {
						friendly.get(i)->move(robot_positions[i].first, robot_positions[i].second, Point());
					}

					current_state=BOT0_PASS;
					break;
				case BOT0_PASS:{
					Point intercept_location = horizontal_intercept(player1);

					if(player0->has_ball()) {
						AI::HL::STP::Action::autokick(player0, Point(), kick_speed);
						kicked_ball = true;
					}
					if(!((intercept_location - Point(0,0)).len() < 1e-9) && (intercept_location.y - player1->position().y < 1e-9) && kicked_ball) {
						player1->move(Point(intercept_location.x, player1->position().y), robot_positions[1].second, Point());
					}

					std::cout<<intercept_location<<std::endl;
					if(player1->has_ball()){
						current_state = BOT1_PASS;
						kicked_ball = false;
					}
				}
					break;
				case BOT1_PASS:{
					Point intercept_location = horizontal_intercept(player2);

					if(player1->has_ball()){
						player1->move(player1->position(),robot1_orientation_dos,Point());
						AI::HL::STP::Action::autokick(player1, Point(), kick_speed);
						kicked_ball=true;
					}
					if(!((intercept_location - Point(0,0)).len() < 1e-9) && (intercept_location.y - player2->position().y < 1e-9) && kicked_ball) {
						player2->move(Point(intercept_location.x, player2->position().y), robot_positions[2].second, Point());
					}
				}
					break;
				case BOT2_PASS:
					break;
				case BOT3_PASS:
					break;
				case BOT0_REPOS:
					break;
				case BOT1_REPOS:
					break;
				case BOT2_REPOS:
					break;
				case BOT3_REPOS:
					break;
				}

			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			World &world;
			std::vector<std::pair<Point, Angle>> robot_positions;
			state current_state;
			Point horizontal_intercept(Player::Ptr player);
			bool kicked_ball;

};

Point PASCHL::horizontal_intercept(Player::Ptr player){
	double top_horizontal_line=1.2;
	double width_of_rectangle = 2;
	double height_of_rectangle = top_horizontal_line*2;
	Rect ball_intercept_boundary(Point((player->position().x - width_of_rectangle*.5), -top_horizontal_line), height_of_rectangle, width_of_rectangle);

	return vector_rect_intersect(ball_intercept_boundary,world.ball().position(), world.ball().velocity()+ world.ball().position());
}


HIGH_LEVEL_REGISTER(PASCHL)

