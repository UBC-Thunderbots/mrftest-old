#include "geom/angle.h"
#include "world/timestep.h"
#include <cmath>
#include "sim/autoref.h"
#include "sim/simulator.h"
#include "sim/field.h"
#include "ai/strategy/simu_test_strategy.h"
#include <iostream>
namespace simu_test{

//simu_test_strategy::auto_ref_setup;


	class simple_ref : public autoref {
			private:
	simulator &simu;
		simulator_ball_impl::ptr ball;
		std::vector<player_impl::ptr> &w_team;
		std::vector<player_impl::ptr> &e_team;
		xmlpp::Element *xmll;				
	 	field::ptr fld;
		
		public:
		

	
	simple_ref(simulator &sim, simulator_ball_impl::ptr the_ball, std::vector<player_impl::ptr> &west_team, std::vector<player_impl::ptr> &east_team, xmlpp::Element *xml) : simu(sim), ball(the_ball), w_team(west_team),e_team( east_team),xmll( xml)  {
	
	 			field::ptr fldd(new simulator_field);
				 fld = fldd;
			}
	
			void tick() {
				

			if(simu.west_team.get_strategy()->get_factory().name ==	"Simulator Test (For Jason)"){
			
			if(simu_test_strategy::auto_ref_setup){
			
			
			ball->ext_drag(simu_test_strategy::ball_pos, simu_test_strategy::ball_vel);
			simu_test_strategy::auto_ref_setup = false;
			
			}
			
			}
				/*
				if(typeid((simu.west_team.get_strategy())) == typeid(a)){
				
				
				std::cout<<"yay Jason!!!"<<std::endl;
				}
				//west.get_strategy();
				*/
				
				
				/*
				
				if(simu_test_strategy::auto_ref_setup){
				//simu_test_strategy::auto_ref_setup;
				//ball_pos, ball_vel, player_pos;
				
				//point p = simu_test::simu_test_strategy::ball_pos;
				//point v = simu_test::simu_test_strategy::ball_vel;
				//point playp= simu_test::simu_test_strategy::player_pos;
				
					//ball->ext_drag(p, v);
					
					//simu_test::simu_test_strategy::auto_ref_setup=false;
					std::cout<<"hello world!!!"<<std::endl;
					return;
				
				
				}
				*/
				//std::cout<<"iiiii"<<std::endl;
				
				if(ball->in_goal()){
				
				//std::cout<<"jason"<<std::endl;
				
					if(ball->position().x<0){
					 simu.east_team.score++;
					}else{
					 simu.west_team.score++;
					}
					point p(0.0, 0.0);
					point v(0.0, 0.0);
					ball->ext_drag(p, v);
					return;
				}
			
				
				//std::cout<<"jjjjj"<<std::endl;
	
				

				
				
			}

			
			Gtk::Widget *get_ui_controls() {
				return 0;
			}

			autoref_factory &get_factory();

			
	};

	//
	// A factory for creating simple_refs.
	//
	class simple_ref_factory : public autoref_factory {
		public:
			simple_ref_factory() : autoref_factory("Simple Auto Ref") {
			}

			autoref::ptr create_autoref(simulator &sim, simulator_ball_impl::ptr the_ball, std::vector<player_impl::ptr> &west_team, std::vector<player_impl::ptr> &east_team, xmlpp::Element *xml) {
				autoref::ptr p(new simple_ref(sim, the_ball, west_team, east_team, xml));
				return p;
			}
	};


	//
	// The global instance of sim_engine_factory.
	//
	simple_ref_factory fact;

	autoref_factory &simple_ref::get_factory() {
		return fact;
	}	

}

