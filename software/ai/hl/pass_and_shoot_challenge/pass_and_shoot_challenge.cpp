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
using namespace AI::HL::STP;

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
			explicit PASCHL(World world) : world(world) {
				robot_positions.push_back(std::make_pair(bot0_initial, (bot1_initial - bot0_initial).orientation()));
				robot_positions.push_back(std::make_pair(bot1_initial, (bot0_initial - bot1_initial).orientation()));
				robot_positions.push_back(std::make_pair(bot2_initial, (bot1_initial - bot2_initial).orientation()));
				robot_positions.push_back(std::make_pair(bot3_initial, (bot2_initial - bot3_initial).orientation()));

				current_state = INITIAL_POSITION;

				kicked_ball = false;

			}

			HighLevelFactory &factory() const;

			void tick() {
				FriendlyTeam friendly = world.friendly_team();

				if (friendly.size() < min_team_size) {
					return;
				}

				Player player0 = friendly[0];
				Player player1 = friendly[1];
				Player player2 = friendly[2];
				Player player3 = friendly[3];

				switch (current_state) {
				case INITIAL_POSITION:{
					for (unsigned int i = 1; i < min_team_size; i++) {
						friendly[i].move(robot_positions[i].first, robot_positions[i].second, Point());
					}

				    bool bot0_is_init = intercept_and_move(0);
					bool bot1_is_init = player1.position().close(robot_positions[1].first, epsilon);
					bool bot2_is_init = player2.position().close(robot_positions[2].first, epsilon);
					bool bot3_is_init = player3.position().close(robot_positions[3].first, epsilon);

					if (bot0_is_init && bot1_is_init && bot2_is_init && bot3_is_init) {
						current_state = BOT0_PASS;
						std::cout << "next" << std::endl;
					}
				}
					break;
				case BOT0_PASS:
					std::cout << "case 1" << std::endl;
					robot_pass(0, 1, BOT1_PASS);
					break;

				case BOT1_PASS: {
					std::cout << "case 2" << std::endl;
					player0.move(bot0_secondary, player0.orientation(), Point());
					robot_pass(1, 2, BOT2_PASS);
				}
					break;
				case BOT2_PASS: {
					std::cout << "case 3" << std::endl;
					player1.move(bot1_secondary, player1.orientation(), Point());
					robot_pass(2, 3, BOT3_PASS);
				}
					break;
				case BOT3_PASS: {
					std::cout << "case 4" << std::endl;
					player2.move(bot2_secondary, player2.orientation(), Point());
					robot_pass(3, 0, BOT0_REPOS);
				}
					break;
				case BOT0_REPOS: {
					std::cout << "case 5" << std::endl;
					player3.move(bot3_secondary, player3.orientation(), Point());
					robot_pass(0, 1, BOT1_REPOS);
				}
					break;
				case BOT1_REPOS: {
					std::cout << "case 6" << std::endl;
					robot_pass(1, 2, BOT2_REPOS);
				}
					break;
				case BOT2_REPOS: {
					std::cout << "case 7" << std::endl;
					robot_pass(2, 3, BOT3_REPOS);
				}
					break;
				case BOT3_REPOS:
					std::cout << "case 8" << std::endl;

					Angle facing_goal = (world.field().enemy_goal() - player3.position()).orientation();

					if (player3.has_ball() && player3.orientation().angle_diff(facing_goal) > Angle::of_degrees(2))
						player3.move(player3.position(), facing_goal , Point());
					else
						player3.autokick(kick_speed);
					break;
				}
			}

			Gtk::Widget *ui_controls() {
				return nullptr;
			}

		private:
			World world;

			std::vector<std::pair<Point, Angle>> robot_positions;
			state current_state;
			Point horizontal_intercept(Player player);
			void robot_pass(std::size_t passer_num, std::size_t receiver_num, state next_state);
			bool ball_out_of_play();
			bool kicked_ball;
		    bool intercept_and_move(std::size_t idx);
};

Point PASCHL::horizontal_intercept(Player player) {
	double horizontal_line = 1.2;
	double width_of_rectangle = 2;
	double height_of_rectangle = horizontal_line * 2;

	if (player.position().y < 0) {
		horizontal_line = -1.2;
	}
	Point sw_corner((player.position().x - width_of_rectangle * .5), -horizontal_line);
	Rect ball_intercept_boundary(sw_corner, height_of_rectangle, width_of_rectangle);

	return vector_rect_intersect(ball_intercept_boundary, world.ball().position(), world.ball().velocity() + world.ball().position());
}

void PASCHL::robot_pass(std::size_t passer_num, std::size_t receiver_num, state next_state) {
	// we are assuming that we have the ball here
	Player passer = world.friendly_team()[passer_num];
	Player receiver = world.friendly_team()[receiver_num];
	Angle passer_orientation = (passer.position() - receiver.position()).orientation();


	//if the ball hasn't been kicked yet and passer does not have the ball, intercept to ball
	if (!kicked_ball) {
		if (!passer.has_ball() && !kicked_ball) {
			Action::intercept(passer, receiver.position());
		} else {
			// rotate to face the receiver
			passer.move(passer.position(), passer_orientation, Point());
		}

		Angle acceptable_angle_difference = Angle::of_degrees(10);
		if (passer.orientation().angle_diff(passer_orientation) < acceptable_angle_difference) {
			passer.autokick(kick_speed);
			kicked_ball = true;
		}
	} else {
		if (receiver.has_ball() && kicked_ball) {
			current_state = next_state;
			kicked_ball = false;
			return;
		}

		if (ball_out_of_play()) {
			current_state = INITIAL_POSITION;
			kicked_ball = false;
			return;
		}

		Point intercept_location = horizontal_intercept(receiver);
		bool valid_intercept_location = intercept_location.close(Point());
		bool able_to_intercept = intercept_location.y - receiver.position().y < 1e-9;

		Angle receiver_orientation = (receiver.position() - passer.position()).orientation();
		if (valid_intercept_location && able_to_intercept) {
			receiver.move(Point(intercept_location.x, receiver.position().y), receiver_orientation, Point());

		} else if (receiver.position().close(world.ball().position(), 0.1)) {
			// if the receiver is close to the ball but doesn't have it
			Action::intercept(receiver, world.ball().position());
		}
	}
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

bool PASCHL::intercept_and_move(std::size_t idx) {
  Player intercepter = world.friendly_team()[idx];

  if (!intercepter.has_ball())
    intercepter.move(world.ball().position(), (world.ball().position() - intercepter.position()).orientation(), Point());

  bool at_intercepter_location = intercepter.position().close(robot_positions[0].first, epsilon);

  if (intercepter.has_ball() && !at_intercepter_location)
    intercepter.move(robot_positions[0].first, robot_positions[0].second, Point());

  return intercepter.has_ball() && at_intercepter_location;
}

HIGH_LEVEL_REGISTER(PASCHL)

