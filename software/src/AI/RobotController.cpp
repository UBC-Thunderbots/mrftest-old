#include "AI/CentralAnalyzingUnit.h"
#include "AI/RobotController.h"
#include "datapool/Config.h"
#include "datapool/HWRunSwitch.h"
#include "datapool/World.h"
#include "AI/ControlFilter.h"
#include "XBee/XBeeBot.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <climits>

// Maximum representable values for the setpoints.
// copied from constants.h
#define MAX_SP_VX 2000.0 // speed sideways
#define MAX_SP_VY 5000.0 // speed forward
#define MAX_SP_AX 4000.0 // acceleration sideways
#define MAX_SP_AY 10000.0 // acceleration forward
#define MAX_SP_VT   4.0
#define MAX_DIST 1000.0
const double EPS = 1E-9;

namespace {
	bool simulation = false;

	// both friendly and enemy team because simulation needs it
	//const double rotKp = 0.78740157, rotKi = 1/127.0, rotKd = 0, rotDecay = 0.97;
	const double rotKp = 2.0, rotKi = 2/127.0, rotKd = 0.0, rotDecay = 0.97;
	//const double vxKp = 0.15748031, vxKi = 0, vxKd = 0.01, vxDecay = 0.97;
	//const double vyKp = 0.23622047, vyKi = 0, vyKd = 0.01, vyDecay = 0.97;
	const double vxKp = 0.285714, vxKi = 1/127.0, vxKd = 0, vxDecay = 0.97;
	const double vyKp = 0.71428, vyKi = 1/127.0, vyKd = 0, vyDecay = 0.97;
	//const double fastMoveYAData[] = {1.0, -0.2234};
	//const double fastMoveYBData[] = {0.0213, -0.0208};
	//const double slowMoveYAData[] = {1.0, -0.8530};
	//const double slowMoveYBData[] = {5.0377e-4 -4.9289e-4};
	//static const double TA[] = {1.0, 0.4566, -0.5434};
	//static const double TB[] = {443.1705, -669.386, 270.312};
	//const std::vector<double> fastMoveYA(fastMoveYAData, fastMoveYAData + sizeof(fastMoveYAData) / sizeof(*fastMoveYAData));
	//const std::vector<double> fastMoveYB(fastMoveYBData, fastMoveYBData + sizeof(fastMoveYBData) / sizeof(*fastMoveYBData));
	//const std::vector<double> slowMoveYA(slowMoveYAData, slowMoveYAData + sizeof(slowMoveYAData) / sizeof(*slowMoveYAData));
	//const std::vector<double> slowMoveYB(slowMoveYBData, slowMoveYBData + sizeof(slowMoveYBData) / sizeof(*slowMoveYBData));
	std::vector<PID> rotFilter(2 * Team::SIZE, PID(rotKp, rotKi, rotKd, rotDecay));
	std::vector<PID> vxFilter(2 * Team::SIZE, PID(vxKp, vxKi, vxKd, vxDecay));
	std::vector<PID> vyFilter(2 * Team::SIZE, PID(vyKp, vyKi, vyKd, vyDecay));
	//std::vector<MoveFilter> vxFilter(2 * Team::SIZE, MoveFilter(moveKA, moveKB));
	//std::vector<MoveFilter> vyFastFilter(2 * Team::SIZE, MoveFilter(fastMoveYA,fastMoveYB));
	//std::vector<MoveFilter> vySlowFilter(2 * Team::SIZE, MoveFilter(slowMoveYA,slowMoveYB));
	
	//acc.length() should always be less than 1
	void simulateWorld(PPlayer robot, Vector2 acc, double rotate, unsigned char kick, bool equalSpeed /*circular speed profile if true*/) {
		if (acc.length() > 1) acc /= acc.length();
		if (equalSpeed){
			acc *= World::get().field().convertMmToCoord(MAX_SP_VX) / CentralAnalyzingUnit::FRAMES_PER_SECOND;
			Vector2 currvel = robot->simulatedVelocity();
			Vector2 newacc = acc - currvel;
			Vector2 newaccperp = acc*currvel.length()/(acc.length()+EPS) - currvel;
			Vector2 newaccpar = newacc - newaccperp;
			double newacclen = World::get().field().convertMmToCoord(MAX_SP_AX) /	(CentralAnalyzingUnit::FRAMES_PER_SECOND*CentralAnalyzingUnit::FRAMES_PER_SECOND);
			if (newacc.length() > newacclen){
				newacc = newacc / newacc.length() * newacclen;
			}
			robot->simulatedAcceleration(newacc);
		}
		else{
			double rot = robot->orientation() * M_PI / 180.0;
			Vector2 rotated(acc.x * std::cos(rot) + acc.y * std::sin(rot), -acc.x * std::sin(rot) + acc.y * std::cos(rot));
			double rotatedangle = rotated.angle() * M_PI / 180.0;
			rotated *= World::get().field().convertMmToCoord(std::min(MAX_SP_VY/(std::abs(std::cos(rotatedangle))+ EPS),MAX_SP_VX/(std::abs(std::sin(rotatedangle))+ EPS))) / CentralAnalyzingUnit::FRAMES_PER_SECOND;

			Vector2 currvel = robot->simulatedVelocity();
			Vector2 velrotated(currvel.x * std::cos(rot) + currvel.y * std::sin(rot), currvel.x * -std::sin(rot) + currvel.y * std::cos(rot));
			Vector2 accrotated = rotated - velrotated;
			double accrotatedangle = accrotated.angle() * M_PI / 180.0;
			double newacclen = World::get().field().convertMmToCoord(std::min(MAX_SP_AY/(std::abs(std::cos(accrotatedangle))+ EPS),MAX_SP_AX/(std::abs(std::sin(accrotatedangle))+ EPS))) /(CentralAnalyzingUnit::FRAMES_PER_SECOND*CentralAnalyzingUnit::FRAMES_PER_SECOND);
			if (accrotated.length() > newacclen){
				accrotated = accrotated / accrotated.length() * newacclen;
			}

			Vector2 newacc(accrotated.x * std::cos(-rot) + accrotated.y * std::sin(-rot), accrotated.x * -std::sin(-rot) + accrotated.y * std::cos(-rot));
			robot->simulatedAcceleration(newacc);
		}

		double angle = robot->orientation();

		double rotationSpeed = MAX_SP_VT * 180.0 / M_PI / CentralAnalyzingUnit::FRAMES_PER_SECOND;
		//int rotationSpeed = 10;

		if (std::fabs(angle - rotate) < rotationSpeed) {
			angle = rotate;
		} else {
			double clockwise;
			if (rotate > angle) clockwise = rotate - angle;
			else                clockwise = 360 - (angle - rotate);

			if (clockwise > 180)
				if (angle - rotationSpeed > 0)
					angle -= rotationSpeed;
				else
					angle += 360 - rotationSpeed;
			else

				if (angle + rotationSpeed < 360)
					angle += rotationSpeed;
				else
					angle += rotationSpeed - 360;
		}

		robot->orientation(angle);

		if (kick && angle == rotate) {
			robot->hasBall(false);
			Vector2 vel = rotate;
			vel *= 5;
			World::get().ball().simulatedVelocity(vel);
		}
	}

	template<typename ret, ret min, ret max>
	ret clamp(double in) {
		if (in < min)
			return min;
		else if (in > max)
			return max;
		else
			return static_cast<ret>(in);
	}

	void sendWireless(PPlayer robot, Vector2 error, double rotate, double dribble, double kick) {
		Vector2 convertedError = World::get().field().convertCoordToMm(error);
		
		
		
		std::ostringstream oss;
		oss << "AI" << robot->id();
		const std::string &key = oss.str();
		if (!Config::instance().hasKey("AI2XBee", key))
			return;
		unsigned int index = Config::instance().getInteger<unsigned int>("AI2XBee", key, 10);

		// Rotate X and Y to be relative to the robot, not the world!
		//
		// AI works in GTK+ coordinates: +X is east, +Y is south
		// Robot works in robot-relative coordinates: +X is right, +Y is forward
		//
		// If robot is facing east:
		//   Orientation = 0
		//   World X ->  Robot Y
		//   World Y ->  Robot X
		// If robot is facing north:
		//   Orientation = 90
		//   World X ->  Robot X
		//   World Y -> -Robot Y
		// If robot is facing west:
		//   Orientation = 180
		//   World X -> -Robot Y
		//   World Y -> -Robot X
		// If robot is facing south:
		//   Orientation = 270
		//   World X -> -Robot X
		//   World Y ->  Robot Y

		
		double rot = robot->orientation() * M_PI / 180.0;
		Vector2 rotated(convertedError.x * std::sin(rot) + convertedError.y * std::cos(rot), convertedError.x * std::cos(rot) + convertedError.y * -std::sin(rot));
		//Vector2 mea(robot->velocity());
		//Vector2 mrotate(mea.x * std::sin(rot) + mea.y * std::cos(rot), mea.x * std::cos(rot) + mea.y * -std::sin(rot));
		//mrotate.x = World::get().field()->convertCoordToMm(mrotate.x) / CentralAnalyzingUnit::FRAMES_PER_SECOND;
		//mrotate.y = World::get().field()->convertCoordToMm(mrotate.y) / CentralAnalyzingUnit::FRAMES_PER_SECOND;

		Glib::RefPtr<XBeeBot> bot = XBeeBotSet::instance()[index];

		bot->vx(vxFilter[robot->id()].process( rotated.x ) / MAX_SP_VX);
		bot->vy(vyFilter[robot->id()].process( rotated.y ) / MAX_SP_VY);
		
		//bot->vx(rotated.x / MAX_SP_VX * 0.5);
		//bot->vy(rotated.y / MAX_SP_VY * 0.5);

		
		double diff = rotate - robot->orientation();
		while (diff >= 180)  diff -= 360;
		while (diff <= -180) diff += 360;
		bot->vt(rotFilter[robot->id()].process(diff / 180.0 * M_PI) / MAX_SP_VT);
		
		
		//bot->vt(0);
		/*if (robot->id() == 0) {
			Vector2 position = World::get().field()->convertMmToCoord(robot->position());
			std::cout << "" << position.x << "\t" << position.y << "\t" << robot->orientation() << std::endl;
			//bot->vt(0);
			bot->vx(-0.1);
			bot->vy(0);
		}*/
		
		//double out = rotFilter[robot->id()].process(-diff / 180.0 * M_PI) / MAX_SP_VT;
		//bot->vt(out);
		//std::cout << robot->id() << "\t rotate=" << rotate << "\t orient=" << robot->orientation() << "\t diff=" << diff << "\t out=" << out << std::endl;
		//if (robot->id() == 4) {
		//	std::cout << std::endl;
		//}
		//if(robot->id() == 0)
			//std::cout << -diff << std::endl;

		//{
			//Vector2 cur(robot->orientation());
			//Vector2 tgt(rotate);
			//diff = cur.angle() - tgt.angle();
			//while (diff >= 180)  diff -= 360;
			//while (diff <= -180) diff += 360;
			//bot->vt(rotFilter[robot->id()].process(-diff / 180));
		//}

		bot->dribbler(dribble);
		if (kick)
			bot->kick(kick);

		if (!bot->run()) {
			rotFilter[robot->id()].clear();
			vxFilter[robot->id()].clear();
			vyFilter[robot->id()].clear();
		}
	}
}

void RobotController::setSimulation(bool sim) {
	simulation = sim;
}

void RobotController::sendCommand(PPlayer robot, Vector2 acc, double rotate, unsigned char dribble, unsigned char kick) {
	// Cap magnitude of acc.
	Vector2 error = acc - robot->position();
	if(error.length() > World::get().field().convertMmToCoord(MAX_DIST))
		error = error / error.length() * World::get().field().convertMmToCoord(MAX_DIST);
		
	//error *= 2.0;
	
	robot->requestedVelocity(error);

	if (simulation)
		simulateWorld(robot, error/World::get().field().convertMmToCoord(MAX_DIST), rotate, kick, false);
	else
		sendWireless(robot, error, rotate, dribble, kick);
}

