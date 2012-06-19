#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/flags.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/intercept.h"
#include "geom/util.h"
#include <iostream>


using namespace AI::HL;
using namespace AI::HL::W;

namespace {
  const unsigned int min_team_size = 4;
  double bot_y_top_position = 1.2;
  double bot_y_bottom_position = -1.2;
  Point bot0_initial(-2.75,bot_y_bottom_position);
  Point bot1_initial(-2.00,bot_y_top_position);
  Point bot2_initial(-1.00,bot_y_bottom_position);
  Point bot3_initial(0.00,bot_y_top_position);
  Point bot0_secondary(0.75, bot_y_bottom_position);
  Point bot1_secondary(1.5, bot_y_top_position);
  Point bot2_secondary(2.25, bot_y_bottom_position);
  Point bot3_secondary(2.75, bot_y_top_position);  
  double kick_speed = 0.05;
  double epsilon = 0.1;
  

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

				robot_orientation[0] = (bot1_initial - bot0_initial).orientation();
				robot_orientation_passing[0] = (bot1_initial - bot0_initial).orientation();
				robot_orientation_final[0] = (bot3_initial - bot0_secondary).orientation();
				robot_orientation_final_passing[0] = (bot1_secondary - bot0_secondary).orientation();

				robot_orientation[1] = (bot0_initial - bot1_initial).orientation();
				robot_orientation_passing[1] = (bot2_initial - bot1_initial).orientation();
				robot_orientation_final[1] = (bot0_secondary - bot1_secondary).orientation();
				robot_orientation_final_passing[1] = (bot2_secondary - bot1_secondary).orientation();

				robot_orientation[2] = (bot1_initial - bot2_initial).orientation();
				robot_orientation_passing[2] = (bot3_initial - bot2_initial).orientation();
				robot_orientation_final[2] = (bot1_secondary - bot2_secondary).orientation();
				robot_orientation_final_passing[2] = (bot3_secondary - bot2_secondary).orientation();

				robot_orientation[3] = (bot2_initial - bot3_initial).orientation();
				robot_orientation_passing[3] = (bot0_secondary - bot3_initial).orientation();
				robot_orientation_final[3] = (bot2_secondary - bot3_secondary).orientation();
				robot_orientation_final_passing[3] = (world.field().enemy_goal() - bot3_secondary).orientation();

				robot_positions.push_back(std::make_pair(bot0_initial, robot_orientation[0]));
				robot_positions.push_back(std::make_pair(bot1_initial,robot_orientation[1]));
				robot_positions.push_back(std::make_pair(bot2_initial,robot_orientation[2]));
				robot_positions.push_back(std::make_pair(bot3_initial,robot_orientation[3]));

				current_state = INITIAL_POSITION;

				kicked_ball = false;

			}

			HighLevelFactory &factory() const;

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();

				if (friendly.size() < min_team_size) {
					return;
				}

				Player::Ptr player0 = friendly.get(0);
				Player::Ptr player1 = friendly.get(1);
				Player::Ptr player2 = friendly.get(2);
				Player::Ptr player3 = friendly.get(3);

				switch (current_state) {
				case INITIAL_POSITION:{
					for (unsigned int i = 1; i < min_team_size; i++) {
						friendly.get(i)->move(robot_positions[i].first, robot_positions[i].second, Point());
					}

				    bool bot0_is_init = intercept_and_move(0);
					bool bot1_is_init = player1->position().close(robot_positions[1].first, epsilon);
					bool bot2_is_init = player2->position().close(robot_positions[2].first, epsilon);
					bool bot3_is_init = player3->position().close(robot_positions[3].first, epsilon);



					if (bot0_is_init && bot1_is_init && bot2_is_init && bot3_is_init) {
						current_state = BOT0_PASS;
						std::cout << "next" << std::endl;
					}
				}
					break;
				case BOT0_PASS:
					std::cout << "case 1" << std::endl;
					robot_pass(0,1, BOT1_PASS, Angle(robot_orientation_passing[0]));
					break;

				case BOT1_PASS: {
					std::cout << "case 2" << std::endl;
					player0->move(Point(bot0_secondary), Angle(robot_orientation_final[0]), Point());
					robot_pass(1, 2, BOT2_PASS, Angle(robot_orientation_passing[1]));
				}
					break;
				case BOT2_PASS: {
					std::cout << "case 3" << std::endl;
					player1->move(Point(bot1_secondary), Angle(robot_orientation_final[1]), Point());
					robot_pass(2, 3, BOT3_PASS, Angle(robot_orientation_passing[2]));
				}
					break;
				case BOT3_PASS: {
					std::cout << "case 4" << std::endl;
					player2->move(Point(bot2_secondary), Angle(robot_orientation_final[2]), Point());
					robot_pass(3, 0, BOT0_REPOS, Angle(robot_orientation_passing[3]));
				}
					break;
				case BOT0_REPOS: {
					std::cout << "case 5" << std::endl;
					player3->move(Point(bot3_secondary), Angle(robot_orientation_final[3]), Point());
					robot_pass(0, 1, BOT1_REPOS, Angle(robot_orientation_final_passing[0]));
				}
					break;
				case BOT1_REPOS: {
					std::cout << "case 6" << std::endl;
					robot_pass(1, 2, BOT2_REPOS, Angle(robot_orientation_final_passing[1]));
				}
					break;
				case BOT2_REPOS: {
					std::cout << "case 7" << std::endl;
					robot_pass(2, 3, BOT3_REPOS, Angle(robot_orientation_final_passing[2]));
				}
					break;
				case BOT3_REPOS:
					std::cout << "case 8" << std::endl;
					if (player3->has_ball())
						player3->autokick(kick_speed);
					break;
				}
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			World &world;
			// Orientation robot is for receiving ball
			Angle robot_orientation[4];
			//Orientation robot is in for passing ball
			Angle robot_orientation_passing[4];
			//second position orientation for receiving ball
			Angle robot_orientation_final[4];
			//second position orientation for passing ball
			Angle robot_orientation_final_passing[4];

			std::vector<std::pair<Point, Angle>> robot_positions;
			state current_state;
			Point horizontal_intercept(Player::Ptr player);
			void robot_pass(int passer_num, int receiver_num, state next_state, Angle orientation);
			void robot_pass_repos(Player::Ptr passer, Player::Ptr receiver, int robot_number);
			bool ball_out_of_play();
			bool kicked_ball;
		    bool intercept_and_move(int idx);
};

Point PASCHL::horizontal_intercept(Player::Ptr player) {
	double horizontal_line = 1.2;
	double width_of_rectangle = 2;
	double height_of_rectangle = horizontal_line * 2;

	if (player->position().y < 0) {
		horizontal_line = -1.2;
	}
	Point sw_corner((player->position().x - width_of_rectangle * .5), -horizontal_line);
	Rect ball_intercept_boundary(sw_corner, height_of_rectangle, width_of_rectangle);

	return vector_rect_intersect(ball_intercept_boundary, world.ball().position(), world.ball().velocity() + world.ball().position());
}

void PASCHL::robot_pass(int passer_num, int receiver_num, state next_state, Angle orientation) {
	Player::Ptr passer = world.friendly_team().get(passer_num);
	Player::Ptr receiver = world.friendly_team().get(receiver_num);
	Point intercept_location = horizontal_intercept(receiver);

	if ((passer->position().len() - world.ball().position().len()) < 0.01)
		AI::HL::STP::Action::intercept(passer, world.ball().position());

	Angle acceptable_angle_difference = Angle::of_degrees(2);

	if (passer->has_ball())
		passer->move(passer->position(), orientation, Point());


	if (passer->has_ball() && ((passer->orientation().angle_diff(orientation)) < acceptable_angle_difference)) {
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

	if (receiver->has_ball()) {
		current_state = next_state;
		kicked_ball = false;
	}

	if (ball_out_of_play())
		current_state = INITIAL_POSITION;
}

bool PASCHL::ball_out_of_play() {

	Point ball_position = world.ball().position();
	Point ball_velocity = world.ball().velocity();
	double ball_future = ball_position.y + ball_velocity.y * 2;
	bool ball_out_bounds_x = ball_position.x < world.field().friendly_goal().x || ball_position.x > world.field().enemy_goal().x;
	bool ball_out_bounds_y = ball_position.y < -(world.field().width() / 2) || ball_position.y > (world.field().width() / 2);
	bool ball_position_in_square = ball_position.y < 0.8 && ball_position.y > -0.8;
	
	//if ball is out of bounds, return true
	if (ball_out_bounds_x || ball_out_bounds_y)
		return true;
	//if ball is inside the square and is moving less than 0.5, return true
	return (ball_position_in_square && (ball_future < bot_y_top_position && ball_future > bot_y_bottom_position));
}

bool PASCHL::intercept_and_move(int idx){
  Player::Ptr intercepter= world.friendly_team().get(idx);

  if (!intercepter->has_ball())
    intercepter->move(world.ball().position(), (world.ball().position() - intercepter->position()).orientation(), Point());

  bool intercepter_location = intercepter->position().close(robot_positions[0].first, epsilon);

  if (intercepter->has_ball() && !intercepter_location)
    intercepter->move(robot_positions[0].first, robot_positions[0].second, Point());

  if (intercepter->has_ball() && intercepter_location)
    return true;
  else
    return false;
}

HIGH_LEVEL_REGISTER(PASCHL)

