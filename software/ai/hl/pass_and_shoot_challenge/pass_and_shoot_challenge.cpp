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
	Point bot0_secondary(0.75, -1.2);
	Point bot1_secondary(1.5, 1.2);
	Point bot2_secondary(2.25, -1.2);
	Point bot3_secondary(2.75, 1.2);

	double kick_speed = 0.05;

	enum state{
		/*
		 * Set up bots to initial positions.
			Returns to this state if things go wrong
		*/
		INITIAL_POSITION,
		/*
		 * Bot0 waits until it has possession of ball. it then passes ball to Bot1.
		 * Bot1 moves horizontally along a line and intercepts ball.
		 */
		BOT0_PASS,
		/*
		 * Bot1 rotates and faces Bot2.
		 * if Bot1 has possession of ball, pass to Bot2
		 */
		BOT1_PASS,
		/*
		 * Bot2 rotates and faces Bot3
		 * if Bot2 has possession of ball, pass to Bot3
		 * on next_state, Bot0 goes to position
		 */
		BOT2_PASS,
		/*
		 * Bot3 rotates to face bot0
		 * if bot3 has possession of ball, pass to bot0
		 * on next state, bot1 goes to its next position
		 */
		BOT3_PASS,
		/*
		 * bot0 rotates to face bot1
		 * if b0t0 has possession of ball, pass to bot1
		 * on next state, bot2 goes to its new position
		 */
		BOT0_REPOS,
		/*
		 * bot1 rotates to face bot2
		 * if bot1 still has possession of ball, pass to bot2
		 * on next state, bot3 goes to its new position
		 */
		BOT1_REPOS,
		/*
		 * bot2 rotates to face bot3
		 * if bot2 still has possession of ball, pass to bot3
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

				robot0_orientation = (bot1_initial - bot0_initial).orientation();
				robot0_orientation_final = (bot3_initial - bot0_secondary).orientation();
				robot1_orientation = (bot0_initial - bot1_initial).orientation();
				robot1_orientation_final = (bot0_secondary - bot1_secondary).orientation();
				robot2_orientation = (bot1_initial - bot2_initial).orientation();
				robot2_orientation_final = (bot1_secondary - bot2_secondary).orientation();
				robot3_orientation = (bot2_initial - bot3_initial).orientation();
				robot3_orientation_final = (world.field().enemy_goal() - bot3_secondary).orientation();

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
				Player::Ptr player3 = friendly.get(3);

				switch(current_state) {
				case INITIAL_POSITION:
					for(unsigned int i = 0; i < min_team_size; i++) {
						friendly.get(i)->move(robot_positions[i].first, robot_positions[i].second, Point());
					}

					current_state = BOT0_PASS;
					break;
				case BOT0_PASS:
					robot_pass(0, 1, BOT1_PASS);
					break;
				case BOT1_PASS:
					player0->move(Point(bot0_secondary), Angle(robot0_orientation_final), Point());
					robot_pass(1, 2, BOT2_PASS);
					break;
				case BOT2_PASS:
					player1->move(Point(bot1_secondary), Angle(robot1_orientation_final), Point());
					robot_pass(2, 3, BOT3_PASS);
					break;
				case BOT3_PASS:
					player2->move(Point(bot2_secondary), Angle(robot2_orientation_final), Point());
					robot_pass(3, 0, BOT0_REPOS);
					break;
				case BOT0_REPOS:{

					player3->move(Point(bot3_secondary), Angle(robot3_orientation_final), Point());

					Point intercept_location = horizontal_intercept(player0);

					if(player0->has_ball()){
						player3->move(Point(bot3_secondary),robot3_orientation_final,Point());
						player0->autokick(kick_speed);
						kicked_ball=true;
					}
					if(!((intercept_location - Point(0,0)).len() < 1e-9) && (intercept_location.y - player1->position().y < 1e-9) && kicked_ball) {
						player1->move(Point(intercept_location.x, player1->position().y), robot_positions[1].second, Point());
					}
					if(player1->has_ball()) {
						current_state = BOT1_PASS;
						kicked_ball = false;
					}
				}
					break;
				case BOT1_REPOS:{
				Point intercept_location = horizontal_intercept(player2);

				if(player1->has_ball()){
					player1->autokick(kick_speed);
					kicked_ball=true;
				}
				if(!((intercept_location - Point(0,0)).len() < 1e-9) && (intercept_location.y - player2->position().y < 1e-9) && kicked_ball) {
					player2->move(Point(intercept_location.x, player2->position().y), robot_positions[2].second, Point());
				}
				if(player1->has_ball()) {
					current_state = BOT1_PASS;
					kicked_ball = false;
				} /*else {
					current_state = INITIAL_POSITION;
					kicked_ball = false;
				}*/				}
					break;
				case BOT2_REPOS:{
					Point intercept_location = horizontal_intercept(player3);

				if(player2->has_ball()){
					player2->autokick(kick_speed);
					kicked_ball=true;
				}
				if(!((intercept_location - Point(0,0)).len() < 1e-9) && (intercept_location.y - player3->position().y < 1e-9) && kicked_ball) {
					player3->move(Point(intercept_location.x, player3->position().y), robot_positions[2].second, Point());
				}
				if(player3->has_ball()){
					current_state = BOT3_REPOS;
					kicked_ball = false;
				}
				else{
					current_state = INITIAL_POSITION;
					kicked_ball = false;
				}

				}
					break;
				case BOT3_REPOS:{
					if(player3->has_ball()){
						player3->autokick(kick_speed);
				}
				}
					break;
				}

			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			World &world;

			Angle robot0_orientation;
			Angle robot0_orientation_final;
			Angle robot1_orientation;
			Angle robot1_orientation_final;
			Angle robot2_orientation;
			Angle robot2_orientation_final;
			Angle robot3_orientation;
			Angle robot3_orientation_final;

			std::vector<std::pair<Point, Angle>> robot_positions;
			state current_state;
			Point horizontal_intercept(Player::Ptr player);
			void robot_pass(int passer_num, int receiver_num, state next_state);
			void robot_pass_repos(Player::Ptr passer, Player::Ptr receiver, int robot_number);
			bool kicked_ball;

};

Point PASCHL::horizontal_intercept(Player::Ptr player) {
	double horizontal_line=1.2;
	double width_of_rectangle = 2;
	double height_of_rectangle = horizontal_line * 2;

	if (player->position().y < 0) {
		horizontal_line = -1.2;
	}
	Rect ball_intercept_boundary(Point((player->position().x - width_of_rectangle * .5), - horizontal_line), height_of_rectangle, width_of_rectangle);

	return vector_rect_intersect(ball_intercept_boundary, world.ball().position(), world.ball().velocity() + world.ball().position());
}

void PASCHL::robot_pass(int passer_num, int receiver_num, state next_state) {
	Player::Ptr passer = world.friendly_team().get(passer_num);
	Player::Ptr receiver = world.friendly_team().get(receiver_num);
	Point intercept_location = horizontal_intercept(receiver);

	if (passer->has_ball()) {
		passer->autokick(kick_speed);
		kicked_ball = true;
	}
	// if there is no intercept location, will return (0,0)
	bool valid_intercept_location = (intercept_location - Point(0,0)).len() > 1e-9;
	//whether they coordinate is along the top rectangle line and not along sides
	bool able_to_intercept = intercept_location.y - receiver->position().y < 1e-9;

	if (valid_intercept_location && able_to_intercept && kicked_ball) {
		receiver->move(Point(intercept_location.x, receiver->position().y), robot_positions[receiver_num].second, Point());
	}

	std::cout<<intercept_location<<std::endl;

	if(receiver->has_ball()) {
		current_state = next_state;
		kicked_ball = false;
	}

}





HIGH_LEVEL_REGISTER(PASCHL)

