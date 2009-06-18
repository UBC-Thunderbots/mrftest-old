#include "AI/CentralAnalyzingUnit.h"
#include "AI/RobotController.h"
#include "datapool/EmergencyStopButton.h"
#include "datapool/World.h"
#include "XBee/XBee.h"

#include <tr1/memory>
#include <cmath>
#include <cassert>
#include <climits>

// Maximum representable values for the setpoints.
// copied from constants.h
#define MAX_SP_VX 2000.0 // speed sideways
#define MAX_SP_VY 2000.0 // speed forward
#define MAX_SP_AX 2000.0 // acceleration sideways
#define MAX_SP_AY 2000.0 // acceleration forward
#define MAX_SP_VT   4.0
const double EPS = 1E-9;

namespace {
	bool simulation = false;

	void simulateWorld(PPlayer robot, Vector2 acc, double rotate, unsigned char kick, bool equalSpeed /*circular speed profile if true*/) {
		if (acc.length() > 1) acc /= acc.length();
		if (equalSpeed){
			acc *= World::get().field()->convertMmToCoord(MAX_SP_VX) / CentralAnalyzingUnit::FRAMES_PER_SECOND;
			Vector2 currvel = robot->velocity();
			Vector2 newacc = acc - currvel;
			Vector2 newaccperp = acc*currvel.length()/(acc.length()+EPS) - currvel;
			Vector2 newaccpar = newacc - newaccperp;
			double newacclen = World::get().field()->convertMmToCoord(MAX_SP_AX) /	(CentralAnalyzingUnit::FRAMES_PER_SECOND*CentralAnalyzingUnit::FRAMES_PER_SECOND);
			if (newacc.length() > newacclen){
				newacc = newacc / newacc.length() * newacclen;
			}
			robot->acceleration(newacc);
		}
		else{
			double rot = robot->orientation() * M_PI / 180.0;
			Vector2 rotated(acc.x * std::cos(rot) + acc.y * std::sin(rot), -acc.x * std::sin(rot) + acc.y * std::cos(rot));
			double rotatedangle = rotated.angle() * M_PI / 180.0;
			rotated *= World::get().field()->convertMmToCoord(std::min(MAX_SP_VY/(std::abs(std::cos(rotatedangle))+ EPS),MAX_SP_VX/(std::abs(std::sin(rotatedangle))+ EPS))) / CentralAnalyzingUnit::FRAMES_PER_SECOND;

			Vector2 currvel = robot->velocity();
			Vector2 velrotated(currvel.x * std::cos(rot) + currvel.y * std::sin(rot), currvel.x * -std::sin(rot) + currvel.y * std::cos(rot));
			Vector2 accrotated = rotated - velrotated;
			double accrotatedangle = accrotated.angle() * M_PI / 180.0;
			double newacclen = World::get().field()->convertMmToCoord(std::min(MAX_SP_AY/(std::abs(std::cos(accrotatedangle))+ EPS),MAX_SP_AX/(std::abs(std::sin(accrotatedangle))+ EPS))) /(CentralAnalyzingUnit::FRAMES_PER_SECOND*CentralAnalyzingUnit::FRAMES_PER_SECOND);
			if (accrotated.length() > newacclen){
				accrotated = accrotated / accrotated.length() * newacclen;
			}

			Vector2 newacc(accrotated.x * std::cos(-rot) + accrotated.y * std::sin(-rot), accrotated.x * -std::sin(-rot) + accrotated.y * std::cos(-rot));
			robot->acceleration(newacc);
		}

		double angle = robot->orientation();

		int rotationSpeed = 10;

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
			World::get().ball()->velocity(vel);
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

	class PID {
	public:
		PID(double Kp, double Ki, double Kd, double decay) : Kp(Kp), Ki(Ki), Kd(Kd), decay(decay), integral(0), hasPrev(false) {
		}

		void clear() {
			integral = 0;
			hasPrev = false;
		}

		double process(double error) {
			integral = integral * decay + error;
			double answer = Kp * error + Ki * integral + (hasPrev ? Kd * (error - prev) : 0);
			prev = error;
			hasPrev = true;
			return answer;
		}

	private:
		const double Kp, Ki, Kd, decay;
		double integral, prev;
		bool hasPrev;
	};

	void sendWireless(PPlayer robot, Vector2 acc, double rotate, unsigned char dribble, unsigned char kick) {
		static const double rotKp = 100, rotKi = 1, rotKd = 0, rotDecay = 0.97;
		static PID rotPIDs[Team::SIZE] = {
			PID(rotKp, rotKi, rotKd, rotDecay),
			PID(rotKp, rotKi, rotKd, rotDecay),
			PID(rotKp, rotKi, rotKd, rotDecay),
			PID(rotKp, rotKi, rotKd, rotDecay),
			PID(rotKp, rotKi, rotKd, rotDecay),
		};
		static const double vxKp = 20, vxKi = 0, vxKd = 0, vxDecay = 0.97;
		static PID vxPIDs[Team::SIZE] = {
			PID(vxKp, vxKi, vxKd, vxDecay),
			PID(vxKp, vxKi, vxKd, vxDecay),
			PID(vxKp, vxKi, vxKd, vxDecay),
			PID(vxKp, vxKi, vxKd, vxDecay),
			PID(vxKp, vxKi, vxKd, vxDecay),
		};
		static const double vyKp = 30, vyKi = 0, vyKd = 0, vyDecay = 0.97;
		static PID vyPIDs[Team::SIZE] = {
			PID(vyKp, vyKi, vyKd, vyDecay),
			PID(vyKp, vyKi, vyKd, vyDecay),
			PID(vyKp, vyKi, vyKd, vyDecay),
			PID(vyKp, vyKi, vyKd, vyDecay),
			PID(vyKp, vyKi, vyKd, vyDecay),
		};

		unsigned int index = UINT_MAX;
		for (unsigned int i = 0; i < Team::SIZE; i++)
			if (robot == World::get().friendlyTeam()->player(i))
				index = i;
		assert(index != UINT_MAX);

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
		Vector2 rotated(acc.x * std::sin(rot) + acc.y * std::cos(rot), acc.x * std::cos(rot) + acc.y * -std::sin(rot));
		Vector2 mea(robot->velocity());
		Vector2 mrotate(mea.x * std::sin(rot) + mea.y * std::cos(rot), mea.x * std::cos(rot) + mea.y * -std::sin(rot));
		mrotate.x = World::get().field()->convertCoordToMm(mrotate.x) / CentralAnalyzingUnit::FRAMES_PER_SECOND;
		mrotate.y = World::get().field()->convertCoordToMm(mrotate.y) / CentralAnalyzingUnit::FRAMES_PER_SECOND;

		XBee::out[index].vx = clamp<signed char, -127, 127>(vxPIDs[index].process(rotated.x));
		XBee::out[index].vy = clamp<signed char, -127, 127>(vyPIDs[index].process(rotated.y));
		double diff;
		{
			Vector2 cur(robot->orientation());
			Vector2 tgt(rotate);
			diff = cur.angle() - tgt.angle();
			while (diff >= 180)  diff -= 360;
			while (diff <= -180) diff += 360;
			XBee::out[index].vt = clamp<signed char, -127, 127>(rotPIDs[index].process(-diff / 180));
		}
		XBee::out[index].dribble    = dribble;
		XBee::out[index].kick       = kick;
		XBee::out[index].vxMeasured = clamp<signed char, -127, 127>(mrotate.x * 127);
		XBee::out[index].vyMeasured = clamp<signed char, -127, 127>(mrotate.y * 127);

		if (XBee::out[index].emergency)
			rotPIDs[index].clear();

		XBee::update();
	}
}

void RobotController::setSimulation(bool sim) {
	simulation = sim;
}

void RobotController::sendCommand(PPlayer robot, Vector2 acc, double rotate, unsigned char dribble, unsigned char kick) {
	// Cap magnitude of acc.
	if (acc.length() > 1)
		acc /= acc.length();

	robot->requestedVelocity(acc);

	if (simulation)
		simulateWorld(robot, acc, rotate, kick, true);
	else
		sendWireless(robot, acc, rotate, dribble, kick);
}

