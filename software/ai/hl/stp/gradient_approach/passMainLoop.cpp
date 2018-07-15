/*
 * passMainLoop.cpp
 *
 *  Created on: 2015-05-23
 *      Author: cheng and james
 */

#include "ai/hl/stp/gradient_approach/passMainLoop.h"
#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/gradient_approach/optimizepass.h"
#include "ai/hl/util.h"
#include "ai/hl/world.h"
#include "geom/angle.h"
#include "geom/point.h"

#include <iostream>

namespace AI {
	namespace HL {
		namespace STP {
			namespace GradientApproach {
				void superLoop() {
					std::cout << "in super loop" << std::endl;
					std::vector<PassInfo::passDataStruct> passPointsLog;
					
					std::cout << "getting world snapshot" << std::endl;
					PassInfo::worldSnapshot snapshot = PassInfo::Instance().getWorldSnapshot();
					
					std::cout << "getting new positions" << std::endl;
					passPointsLog = newPositions(snapshot, 30);
					std::cout << "about to step forward" << std::endl;
					passPointsLog = stepForward(snapshot, passPointsLog);

					while(true){
						std::vector<PassInfo::passDataStruct> newStartingPos = newPositions(snapshot, 4);
						passPointsLog.insert(passPointsLog.end(), newStartingPos.begin(), newStartingPos.end());
						passPointsLog = merge(passPointsLog);

						passPointsLog = stepForward( snapshot, passPointsLog);
						passPointsLog = merge(passPointsLog);

						
						if (passPointsLog.size() > 50) {
							std::sort(passPointsLog.begin(), passPointsLog.end(), comparePassQuality);
							passPointsLog.erase(passPointsLog.begin() + 40 , passPointsLog.end());
						}
						std::vector<PassInfo::passDataStruct> best_points = bestPassPositions(snapshot, passPointsLog, 10);
						PassInfo::Instance().updateCurrentPoints(best_points);
						
						if(best_points.size() > 0){
							if (best_points.at(0).quality < 0.03){
								//std::cout << "clearing pass positions" << std::endl;
								passPointsLog.clear();	
								passPointsLog = newPositions(snapshot, 30);
							}
						}
						//std::cout << "best quality: "<< best_points.at(0).quality;

						snapshot = PassInfo::Instance().getWorldSnapshot();
					}
				}

				void testLoop(PassInfo::worldSnapshot snapshot) {
					std::vector<std::vector<PassInfo::passDataStruct> > passPointsLog;
					std::vector<PassInfo::passDataStruct> newStartingPos = newPositions(snapshot, 100);

					if(passPointsLog.size() == 0){
						passPointsLog.push_back(newStartingPos);
					}
					else{
						passPointsLog.back().insert(passPointsLog.back().end(), newStartingPos.begin(), newStartingPos.end());
					}

					passPointsLog.push_back(merge(passPointsLog.back()));
					std::vector<PassInfo::passDataStruct> steppedData = stepForward(snapshot, passPointsLog.back());
					passPointsLog.back() = steppedData;
					passPointsLog.back() = merge(passPointsLog.back());
					PassInfo::Instance().updateCurrentPoints(passPointsLog.back());
				}


				std::vector<PassInfo::passDataStruct> newPositions(PassInfo::worldSnapshot snapshot, unsigned int quantity){
					// Generate a list of potential points, then randomly select the number specified by 'quantity' to be returned.
					// Current implementation is fairly inefficient when 'quantity' is smaller than the number of potential points.
					// An improvement would be to randomly pick indices then calculate only their positions.
					const Angle ENEMY_ORIENTATION_THRESHHOLD = Angle::of_degrees(1);
					const double FRIENDLY_RADIUS_THRESHHOLD = 0.1;
					std::vector<PassInfo::passDataStruct> startingPositions;
					std::vector<Point> mergedEnemyPositions;
					std::vector<double> friendlyRadii;
					bool foundCloseCopy;

					if(snapshot.enemy_positions.size() > 0){

						mergedEnemyPositions.push_back((snapshot.enemy_positions.at(0)-snapshot.passer_position));


						foundCloseCopy = false;
						for(unsigned int i =1; i < snapshot.enemy_positions.size(); i++){
							foundCloseCopy = false;
							Point each_enemyPosition = (snapshot.enemy_positions.at(i)-snapshot.passer_position);
							for(unsigned int j = 0; j < mergedEnemyPositions.size(); j++){
								if((each_enemyPosition.orientation().angle_diff(mergedEnemyPositions.at(j).orientation())) < ENEMY_ORIENTATION_THRESHHOLD){
									foundCloseCopy = true;
									break;
								}
							}

							if(!foundCloseCopy){
								mergedEnemyPositions.push_back(each_enemyPosition);
								//std::cout << mergedEnemyPositions.back().x << std::endl;
							}

						}


						std::sort (mergedEnemyPositions.begin(), mergedEnemyPositions.end());  


						mergedEnemyPositions.push_back(mergedEnemyPositions.at(0));
					}
					if(snapshot.passee_positions.size() > 0){
						friendlyRadii.push_back((snapshot.passee_positions.at(0)-snapshot.passer_position).len());

						for (unsigned int i = 1; i < snapshot.passee_positions.size(); i++){
							foundCloseCopy = false;
							double potential_radius = (snapshot.passee_positions.at(i)-snapshot.passer_position).len();
							for(unsigned int j = 0; j < friendlyRadii.size(); j++){
								if(fabs(friendlyRadii.at(j)-potential_radius) < FRIENDLY_RADIUS_THRESHHOLD){
									foundCloseCopy= true;
									break;

								}

							}
							if(!foundCloseCopy){
								friendlyRadii.push_back((snapshot.passee_positions.at(i)-snapshot.passer_position).len());
							}
						}

						for (unsigned int i = 0; i < snapshot.passee_positions.size(); i++){
							/*
							PassInfo::passDataStruct lDataStruct;
							lDataStruct.params.push_back(snapshot.passee_positions.at(i).x);
							lDataStruct.params.push_back(snapshot.passee_positions.at(i).y);
							//This is set to 0 since if we are passing straight on dont need to delay
							lDataStruct.params.push_back(0);
							lDataStruct.params.push_back(4);
							lDataStruct.alpha = 0.5;
							*/
							startingPositions.push_back(PassInfo::passDataStruct(snapshot.passee_positions.at(i).x,
									snapshot.passee_positions.at(i).y,
									0.3,4.0,0.6));
						}
					}
					if(mergedEnemyPositions.size() > 1){
						for(unsigned int i =0; i < mergedEnemyPositions.size() -1; i++){

							for(unsigned int j = 0; j < friendlyRadii.size(); j++){

								startingPositions.push_back(estimateParams(snapshot,i, mergedEnemyPositions, j, friendlyRadii));

							}
						}
					}

					if(startingPositions.size() > quantity/2){
						// Randomly order the vector, then truncate the vector to the desired number of points
						std::random_shuffle(startingPositions.begin(), startingPositions.end());

						startingPositions.resize(quantity/2);
					}

					while(startingPositions.size() < quantity){
						//TODO: use defined constants instead of magic numbers
                        double x = ((double) rand() / (RAND_MAX)) * (6.0) - 3.0 ;
                        double y = ((double) rand() / (RAND_MAX)) * (4.0) - 2.0 ;
                        double t = ((double) rand() / (RAND_MAX)) * (2.5) + 0.5 ;
                        double v = ((double) rand() / (RAND_MAX)) * (3.5) + 2.5 ;
						startingPositions.push_back(PassInfo::passDataStruct(x, y, t, v, 0.5));
					}
					//Is there the potential for starting positions to have size less than quantity?
					//std::cout << "num starting positions: " << startingPositions.size() << std::endl;
					return startingPositions;
				}

				PassInfo::passDataStruct estimateParams(PassInfo::worldSnapshot snapshot, int i, std::vector<Point> mergedEnemyPositions, int j, std::vector<double> friendlyRadii){
					double V_MAX = 2;
					
					

					Angle targetOrientation = (mergedEnemyPositions.at(i).orientation()+mergedEnemyPositions.at(i+1).orientation())/2;
					Point lDataStructTarget = Point::of_angle(targetOrientation)*friendlyRadii.at(j) + snapshot.passer_position;

					Angle closestEnemyOrientation = targetOrientation.angle_diff(mergedEnemyPositions.at(0).orientation()).abs();
					int closestEnemyIndex = 0;
					for(unsigned int k = 1; k < mergedEnemyPositions.size(); k++){
						Angle lClosestEnemyOrientation = (targetOrientation.angle_diff(mergedEnemyPositions.at(k).orientation())).abs();
						if(lClosestEnemyOrientation < closestEnemyOrientation){
							closestEnemyOrientation = lClosestEnemyOrientation;
							closestEnemyIndex = k;
						}
					}
					double ball_vel = closestEnemyOrientation.to_radians()*mergedEnemyPositions.at(closestEnemyIndex).len()/V_MAX;


					double shortest_dist = 1000;
					double current_dist;
					for(Point each_passee_position : snapshot.passee_positions){
						if(each_passee_position != snapshot.passer_position){
							current_dist = (lDataStructTarget - each_passee_position).len();
							if(current_dist < shortest_dist){
								shortest_dist = current_dist;
							}
						}
					}


					double t_delay = 3.0*shortest_dist/V_MAX - friendlyRadii.at(j)/ball_vel;
					if(t_delay < 0.5){
						t_delay = 0.5;
					}

					/*
					lDataStruct.params.push_back(lDataStructTarget.x);
					std::cout << "     |     X: " << lDataStructTarget.x;
					lDataStruct.params.push_back(lDataStructTarget.y);
					std::cout << "Y: " << lDataStructTarget.y << std::endl;
					lDataStruct.params.push_back(t_delay);
					lDataStruct.params.push_back(ball_vel);
					lDataStruct.alpha = 0.5;
					*/
					return PassInfo::passDataStruct(lDataStructTarget.x, lDataStructTarget.y, t_delay, ball_vel, 0.5);
				}

				PassInfo::passDataStruct stepForward(PassInfo::worldSnapshot snapshot, PassInfo::passDataStruct dataStruct){
					Point start_target = Point(dataStruct.params.at(0),dataStruct.params.at(1));
					int max_func_evals = 40;
					std::vector<double> output = optimizePass(snapshot, start_target, dataStruct.params.at(2), dataStruct.params.at(3), max_func_evals);
					//PassInfo::passDataStruct lDataStruct;
					/*
					lDataStruct.params.at(0) = output.at(1);
					lDataStruct.params.at(1) = output.at(2);
					lDataStruct.params.at(2) = output.at(3);
					lDataStruct.params.at(3) = output.at(4);
					lDataStruct.quality = output.at(0);
					lDataStruct.alpha = output.at(5);
					std::cout << "Optimized4" << std::endl;

					lDataStruct.gradients.at(0) = output.at(6);
					lDataStruct.gradients.at(1) = output.at(7);
					lDataStruct.gradients.at(2) = output.at(8);
					lDataStruct.gradients.at(3) = output.at(9);
					*/

					//std::cout << "dX:" <<  dataStruct.params.at(0) - output.at(1) << " dY:" << dataStruct.params.at(1) - output.at(2) << std::endl;
					//std::cout << "changeQuality" << output.at(0) - dataStruct.quality << std::endl;
					//std::cout << "alpha" << output.at(5) << " gradX: " << output.at(6) << " gradY: " << output.at(7);
					return PassInfo::passDataStruct(output.at(1),
							output.at(2),
							output.at(3),
							output.at(4),
							output.at(5),
							output.at(0),
							output.at(6),
							output.at(7),
							output.at(8),
							output.at(9));
				}

				std::vector<PassInfo::passDataStruct> stepForward(PassInfo::worldSnapshot snapshot,std::vector<PassInfo::passDataStruct> dataStructs){
					std::vector<PassInfo::passDataStruct> lDataStructs;
					for(unsigned int i = 0; i < dataStructs.size(); i++){
						//std::cout << "stepped";
						lDataStructs.push_back(stepForward(snapshot, dataStructs.at(i)));
					}
					
					return lDataStructs;
				}

				std::vector<PassInfo::passDataStruct> merge(std::vector<PassInfo::passDataStruct> potential_passes){
				//Compare all potential pass objects. For any two passes that are found with a similar destination, the lower quality of the two is pruned.
				

			
					const double MERGE_THRESHHOLD = 1.0;
					double x1;
					double y1;
					double xdist;
					double ydist;
					std::vector<unsigned int> removed_objects;

					for (unsigned int i=0; i < potential_passes.size()-1; i++){
						//check if current pass object is contained in remove-vector (if it is it should be ignored)
						
						if(std::find(removed_objects.begin(), removed_objects.end(), i) == removed_objects.end()) {
							x1 = potential_passes.at(i).params.at(1);
							y1 = potential_passes.at(i).params.at(2);


							for (unsigned int n=i+1; n < potential_passes.size(); n++){
								if(std::find(removed_objects.begin(), removed_objects.end(), n) == removed_objects.end()) {
									xdist = x1 - potential_passes.at(n).params.at(1);
									ydist = y1 - potential_passes.at(n).params.at(2);
									double squared_dist = xdist*xdist + ydist*ydist;

									if (squared_dist < MERGE_THRESHHOLD){
										if(potential_passes.at(i).quality > potential_passes.at(n).quality ){
											removed_objects.push_back(n);
										}
										else {
											removed_objects.push_back(i);
											break;
										}
									}
								}
							}

						}
					}
					std::vector<PassInfo::passDataStruct> merged_potential_passes;
					for (unsigned int i=0; i < potential_passes.size(); i++){
						if(std::find(removed_objects.begin(), removed_objects.end(), i) == removed_objects.end()){
							merged_potential_passes.push_back(potential_passes.at(i));
						}
					}
					return merged_potential_passes;
				}
				bool comparePassQuality(PassInfo::passDataStruct a, PassInfo::passDataStruct b){
					return a.quality > b.quality;
				}

				std::vector<PassInfo::passDataStruct> bestPassPositions(PassInfo::worldSnapshot snapshot, std::vector<PassInfo::passDataStruct> potential_passes, unsigned int numberPositions){
					std::vector<PassInfo::passDataStruct> bestPositions;

					if(potential_passes.size() == 0){
						//no input- return empty list
						return bestPositions;
					}
		
					//sort all points by quality
					std::sort(potential_passes.begin(), potential_passes.end(), comparePassQuality);

					//always grab the highest quality point
					bestPositions.push_back(potential_passes.at(0));


					bool blockingPass = false;
					//iterate over the rest of the pass positions
					for(std::size_t ii = 1; ii < potential_passes.size(); ii++){
						blockingPass = false;
						Point relativeTarget = potential_passes.at(ii).getTarget()-snapshot.passer_position;
						for(std::size_t jj = 0; jj < bestPositions.size(); jj++){
							Point relativeBestPos = bestPositions.at(jj).getTarget()-snapshot.passer_position;

							//If its closer to passer and the fastest distance
							if(( (relativeTarget.len() < relativeBestPos.len()) &&
									((relativeTarget.orientation().angle_diff(relativeBestPos.orientation())).to_radians()*relativeTarget.len()<0.2)
								)){
								blockingPass = true;
								break;
							}
						}
						if(!blockingPass){
						
							bestPositions.push_back(potential_passes.at(ii));
						}
						if(bestPositions.size() >= numberPositions){
							break;
						}
					}
					return bestPositions;
				}

			} /* namespace GradientApproach */
		} /* namespace STP */
	} /* namespace HL */
} /* namespace AI */
