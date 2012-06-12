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
  const double epsilon = 0.3;

  Point bot0_left(-2.0, 0.6);
  Point bot0_right(0, 0.6);

  Point bot1_left(0.5, 0.6);
  Point bot1_right(2.5, 0.6);

  Point bot2_left(-2.8, -0.6);
  Point bot2_right(-0.8, -0.6);

  Point bot3_left(0, -0.6);
  Point bot3_right(2.0, -0.6);
  
  enum state{
    INITIAL,
    ROBOTS_ARE_LEFT,
    ROBOTS_ARE_RIGHT
  };

};

class PASCHL_ENEMY : public HighLevel {
public:
	PASCHL_ENEMY(World &world) : world(world) {

		current_state = INITIAL;

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

			switch(current_state){
			case INITIAL:
			  player0->move(bot0_left, Angle(), Point());
			  player1->move(bot1_left, Angle(), Point());
			  player2->move(bot2_left, Angle(), Point());
			  player3->move(bot3_left, Angle(), Point());

			  current_state=ROBOTS_ARE_LEFT;
			  
			  break;

			case ROBOTS_ARE_LEFT:{
			  bool bot0_is_left= (player0->position()-bot0_left).len() < epsilon;
			  bool bot1_is_left= (player1->position()-bot1_left).len() < epsilon;
			  bool bot2_is_left= (player2->position()-bot2_left).len() < epsilon;
			  bool bot3_is_left= (player3->position()-bot3_left).len() < epsilon;

			  if(bot0_is_left && bot1_is_left && bot2_is_left && bot3_is_left){
			    player0->move(bot0_right, Angle(), Point());
			    player1->move(bot1_right, Angle(), Point());
			    player2->move(bot2_right, Angle(), Point());
			    player3->move(bot3_right, Angle(), Point());
			    current_state=ROBOTS_ARE_RIGHT;
			  } else {
			    player0->move(bot0_left, Angle(), Point());
			    player1->move(bot1_left, Angle(), Point());
			    player2->move(bot2_left, Angle(), Point());
			    player3->move(bot3_left, Angle(), Point());

			  }
			  
   			}
			  break;
			

			case ROBOTS_ARE_RIGHT:{
			  bool bot0_is_right= (player0->position()-bot0_right).len() < epsilon;
			  bool bot1_is_right= (player1->position()-bot1_right).len() < epsilon;
			  bool bot2_is_right= (player2->position()-bot2_right).len() < epsilon;
			  bool bot3_is_right= (player3->position()-bot3_right).len() < epsilon;

			  if(bot0_is_right && bot1_is_right && bot2_is_right && bot3_is_right){
			    player0->move(bot0_left, Angle(), Point());
			    player1->move(bot1_left, Angle(), Point());
			    player2->move(bot2_left, Angle(), Point());
			    player3->move(bot3_left, Angle(), Point());
			    current_state=ROBOTS_ARE_LEFT;
			  } else {
			     player0->move(bot0_right, Angle(), Point());
			     player1->move(bot1_right, Angle(), Point());
			     player2->move(bot2_right, Angle(), Point());
			     player3->move(bot3_right, Angle(), Point()); 
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
		state current_state;


};

HIGH_LEVEL_REGISTER(PASCHL_ENEMY)
