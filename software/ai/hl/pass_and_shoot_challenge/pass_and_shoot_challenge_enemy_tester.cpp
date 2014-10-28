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

  Point bot0_left(-2.8, 0.6);
  Point bot0_right(-.2, 0.6);

  Point bot1_left(.2, 0.6);
  Point bot1_right(2.8, 0.6);

  Point bot2_left(-2.8, -0.6);
  Point bot2_right(-.2, -0.6);

  Point bot3_left(.2, -0.6);
  Point bot3_right(2.8, -0.6);  
  
  enum state{
    INITIAL,
    MOVE
  };

};

class PASCHL_ENEMIES final : public HighLevel {
public:
  explicit PASCHL_ENEMIES(World world) : world(world) {
    current_state=INITIAL;
  }
  
  HighLevelFactory &factory() const override;
  
  void tick() override {
    FriendlyTeam friendly = world.friendly_team();

    if (friendly.size() < min_team_size) {
      return;
    }
    
    Player player0 = friendly[0];
    Player player1 = friendly[1];
    Player player2 = friendly[2];
    Player player3 = friendly[3];
    
    switch(current_state){
    case INITIAL:{
      bool bot0_is_left= (player0.position()-bot0_left).len() < epsilon;
      bool bot1_is_left= (player1.position()-bot1_left).len() < epsilon;
      bool bot2_is_left= (player2.position()-bot2_left).len() < epsilon;
      bool bot3_is_left= (player3.position()-bot3_left).len() < epsilon; 

      player0.move(bot0_left, Angle(), Point());
      player1.move(bot1_left, Angle(), Point());
      player2.move(bot2_left, Angle(), Point());
      player3.move(bot3_left, Angle(), Point());
      
      if(bot0_is_left && bot1_is_left && bot2_is_left && bot3_is_left){
	bot0_state=false;
	bot1_state=false;
	bot2_state=false;
	bot3_state=false;
	current_state=MOVE;
      }
	
    }
      break;

    case MOVE:{

      bool bot0_is_right= (player0.position()-bot0_right).len() < epsilon;
      bool bot1_is_right= (player1.position()-bot1_right).len() < epsilon;
      bool bot2_is_right= (player2.position()-bot2_right).len() < epsilon;
      bool bot3_is_right= (player3.position()-bot3_right).len() < epsilon;

      bool bot0_is_left= (player0.position()-bot0_left).len() < epsilon;
      bool bot1_is_left= (player1.position()-bot1_left).len() < epsilon;
      bool bot2_is_left= (player2.position()-bot2_left).len() < epsilon;
      bool bot3_is_left= (player3.position()-bot3_left).len() < epsilon; 
    
      if(!bot0_state)
	player0.move(bot0_right, Angle(), Point());
      else
	player0.move(bot0_left, Angle(), Point());
      
      if(!bot1_state)
	player1.move(bot1_right, Angle(), Point());
      else
	player1.move(bot1_left, Angle(), Point());

      if(!bot2_state)
	player2.move(bot2_right, Angle(), Point());
      else
	player2.move(bot2_left, Angle(), Point());

      if(!bot3_state)
	player3.move(bot3_right, Angle(), Point());
      else
	player3.move(bot3_left, Angle(), Point());


      if(bot0_is_right)
	bot0_state=true;
      if(bot1_is_right)
	bot1_state=true;
      if(bot2_is_right)
	bot2_state=true;
      if(bot3_is_right)
	bot3_state=true;
      
      if(bot0_is_left)
	bot0_state=false;
      if(bot1_is_left)
	bot1_state=false;
      if(bot2_is_left)
	bot2_state=false;
      if(bot3_is_left)
	bot3_state=false;
    }
      break; 
      
    }
  }


    Gtk::Widget *ui_controls() override {
      return nullptr;
    }
  
  private:
    World world;
    state current_state;
    bool bot0_state,bot1_state,bot2_state,bot3_state;
};
  
HIGH_LEVEL_REGISTER(PASCHL_ENEMIES)
