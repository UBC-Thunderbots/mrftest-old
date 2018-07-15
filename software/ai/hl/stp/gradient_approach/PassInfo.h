/*
 * PassInfo.h
 *
 *  Created on: 2015-03-07
 *      Author: cheng
 */

#include <glibmm/ustring.h>
#include <mutex>
#include <thread>
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/gradient_approach/optimizepass.h"
#include "ai/hl/stp/gradient_approach/passMainLoop.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "ai/hl/stp/world.h"
#include <mutex>
#include <thread>  
#include <atomic>  
#include <glibmm/ustring.h>
#include "ai/hl/stp/ui.h"


#ifndef PASSINFO_H_
#define PASSINFO_H_


namespace AI {
	namespace HL {
		namespace STP {
			namespace GradientApproach {
				class PassInfo {
					public:
						int count = 0;
						bool passAttempted;

						struct passDataStruct{
							passDataStruct() : quality(0) {
							};

							passDataStruct(double x, double y, double t_delay, double ball_vel, double lAlpha) : quality(0) {
								params.push_back(x);
								params.push_back(y);
								params.push_back(t_delay);
								params.push_back(ball_vel);
								alpha = lAlpha;
								time = 0;
							};

							passDataStruct(
									double x,
									double y,
									double t_delay,
									double ball_vel,
									double lAlpha,
									double lQuality,
									double dx,
									double dy,
									double dt_delay,
									double dball_vel
							) { 
								params.push_back(x);
								params.push_back(y);
								params.push_back(t_delay);
								params.push_back(ball_vel);
								gradient.push_back(dx);
								gradient.push_back(dy);
								gradient.push_back(dt_delay);
								gradient.push_back(dball_vel);
								alpha = lAlpha;
								quality = lQuality;
								time = 0;
							};

							inline Point getTarget() {
								return Point(params.at(0), params.at(1));
							}

							std::vector<double> params;
							std::vector<double> gradient;
							int long time;
							double alpha;
							double quality;


						};

						struct worldSnapshot {
							double field_width;
							double field_length;
							Point enemy_goal;
							std::pair<Point, Point> enemy_goal_boundary;
							Point friendly_goal;
							std::pair<Point, Point> friendly_goal_boundary;
							Point passer_position;
							Angle passer_orientation;
							Point passer_veolcity;
							std::vector<Point> passee_positions;
							std::vector<Point> passee_velocities;
							std::vector<Point> enemy_positions;
							std::vector<Point> enemy_velocities;
						};

						static PassInfo& Instance();
						// delete copy and move constructors and assign operators
						PassInfo(PassInfo const&) = delete;             // Copy construct
						PassInfo(PassInfo&&) = delete;                  // Move construct
						PassInfo& operator=(PassInfo const&) = delete;  // Copy assign
						PassInfo& operator=(PassInfo&&) = delete;      // Move assign


						worldSnapshot getWorldSnapshot();
						void  updateWorldSnapshot(worldSnapshot new_snapshot);
						worldSnapshot  convertToWorldSnapshot(World world);
						std::vector<passDataStruct> getCurrentPoints();
						passDataStruct getBestPass();
						double ratePass(passDataStruct pass);
						void setAltPasser(Point, Angle);
						void resetAltPasser();
						void updateCurrentPoints(std::vector<passDataStruct> newPoints);
						void setThreadRunning(bool new_val);
						bool threadRunning();


					protected:
						PassInfo();
					private:
						std::thread pass_thread;
						std::mutex thread_running_mutex;
						std::mutex alt_passer_mutex;
						std::mutex world_mutex;
						std::mutex currentPoints_mutex;
						std::vector<passDataStruct> currentPoints;
						worldSnapshot snapshot;
						std::atomic_bool thread_running; 
						bool use_alt_passer = false;
						Point alt_point;
						Angle alt_ori;
				};


			} /* namespace gradientApproach */
		} /* namespace STP */
	} /* namespace HL */
} /* namespace AI */

#endif /* PASSINFO_H_ */
