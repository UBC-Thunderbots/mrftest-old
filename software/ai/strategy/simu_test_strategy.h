#include "ai/strategy/strategy.h"
#include "ai/navigator/testnavigator.h"
#include "ai/tactic/chase.h"
#include "ai/role/role.h"
#include "ai/role/defensive.h"
#include "ai/role/goalie.h"
#include "ai/role/execute_direct_free_kick_enemy.h"
#include "ai/role/offensive.h"
#include "ai/role/execute_direct_free_kick_friendly.h" 
#include "ai/role/pit_stop.h"
#include "ai/role/execute_indirect_free_kick_enemy.h"     
#include "ai/role/prepare_kickoff_enemy.h"
#include "ai/role/execute_indirect_free_kick_friendly.h"  
#include "ai/role/prepare_kickoff_friendly.h"             
#include "ai/role/prepare_penalty_enemy.h"
#include "ai/role/execute_kickoff_friendly.h"             
#include "ai/role/prepare_penalty_friendly.h"
#include "ai/role/execute_penalty_enemy.h"
#include "ai/role/victory_dance.h"
#include "ai/role/execute_penalty_friendly.h" 
 
 // bool auto_ref_setup=false;
 // point ball_pos, ball_vel, player_pos;
 
namespace simu_test{
  

 
  class simu_test_strategy : public strategy {
  public:
    simu_test_strategy(world::ptr world);
    void tick();
    strategy_factory &get_factory();
    Gtk::Widget *get_ui_controls();
    void robot_added(void);
    void robot_removed(unsigned int index, player::ptr r);
    static bool auto_ref_setup;
    static point ball_pos, ball_vel, player_pos;
    
  private:
    //private variables
	const world::ptr the_world;
    static const int WAIT_AT_LEAST_TURN = 5;		// We need this because we don't want to make frequent changes
    static const int DEFAULT_OFF_TO_DEF_DIFF = 1;	// i.e. one more offender than defender
    int test_id;
    int tick_count;
    bool test_done;
    bool test_started;
    bool tc_receive_receiving;
    bool first_tick;
    point stay_here;
    int tc_receive_receive_count;
    bool tests_completed;
    bool is_ball_in_bound();
    bool is_player_in_pos(player::ptr , double , double);
    bool is_ball_in_pos(double , double);
    void finish_test_case();
#warning THIS IS EVIL. If you weren't breaking hierarchy we wouldn't have to use a raw pointer here!
    testnavigator *our_navigator;
    player::ptr the_only_player;
    bool result[6];
    bool print_msg, print_msg2;
  };
  
  //bool simu_test_strategy::auto_ref_setup = false;
  //point simu_test_strategy::ball_pos(0,0), simu_test_strategy::ball_vel(0,0), simu_test_strategy::player_pos(0,0);
  
 
  
  }
  
