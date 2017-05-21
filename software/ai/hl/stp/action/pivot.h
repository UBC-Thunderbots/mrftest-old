#pragma once
#include "ai/hl/stp/action/action.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Pivot around the ball until the player is oriented in the direction of the target.
				 *
				 *  \param[in] target Used to determine the direction the player should be facing when player finishes pivoting.
				 */
				void pivot(caller_t& ca, World world, Player player, Point target, Angle swing, double radius = 0);

			
			
			
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			#warning The following pivot end condition checker may or may-not work. At the time of writing pivot does not work to test it
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				inline void waitForPivot(caller_t& ca, Point destination, Player player) {
					double tolerence = 0.02;            //2cm tolerence
					Point playerDestinationDifference;
					double differenceMagnitude;

					do {
						playerDestinationDifference = player.position() - destination;														//calculates the vector between the player and their destination
						differenceMagnitude = sqrt( pow(playerDestinationDifference.x, 2.0) + pow( playerDestinationDifference.y, 2.0) );	//converts this difference vector to a magnitude
						Action::yield(ca);
					}while (differenceMagnitude > tolerence);																				//compares the distance betweem the robot and its destination to the specified tolerence
				}

				inline void waitForPivot(caller_t& ca, Point destination, Player player, double tolerence) {          
					Point playerDestinationDifference;
					double differenceMagnitude;

					do {
						playerDestinationDifference = player.position() - destination;														//calculates the vector between the player and their destination
						differenceMagnitude =  sqrt( pow(playerDestinationDifference.x, 2.0) + pow( playerDestinationDifference.y, 2.0) );	//converts this difference vector to a magnitude
						Action::yield(ca);
					}while (differenceMagnitude > tolerence);																				//compares the distance betweem the robot and its destination to the specified tolerence
				}
			}
		}
	}
}
