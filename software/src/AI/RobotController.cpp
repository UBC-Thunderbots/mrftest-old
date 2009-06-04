#include "AI/CentralAnalyzingUnit.h"
#include "AI/RobotController.h"
#include "datapool/EmergencyStopButton.h"
#include "datapool/World.h"
#include "XBee/XBee.h"

#include <tr1/memory>
#include <cmath>
#include <cassert>
#include <climits>

namespace {
	bool xbeeInitialized = false;
	bool simulation = false;

	void simulateWorld(PPlayer robot, Vector2 acc, double rotate, unsigned char dribble, unsigned char kick) {
		robot->acceleration(acc);

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
			vel *= 20;
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
		static const double rotKp = 35, rotKi = 1, rotKd = 0, rotDecay = 0.97;
		static PID rotPIDs[Team::SIZE] = {
			PID(rotKp, rotKi, rotKd, rotDecay),
			PID(rotKp, rotKi, rotKd, rotDecay),
			PID(rotKp, rotKi, rotKd, rotDecay),
			PID(rotKp, rotKi, rotKd, rotDecay),
			PID(rotKp, rotKi, rotKd, rotDecay),
		};

		unsigned int index = UINT_MAX;
		for (unsigned int i = 0; i < Team::SIZE; i++)
			if (robot == World::get().friendlyTeam()->player(i))
				index = i;
		assert(index != UINT_MAX);

		if (!xbeeInitialized) {
			XBee::init();
			xbeeInitialized = true;
		}

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
		if (rotated.length())
			rotated /= rotated.length();
		Vector2 mea(robot->velocity());
		Vector2 mrotate(mea.x * std::sin(rot) + mea.y * std::cos(rot), mea.x * std::cos(rot) + mea.y * -std::sin(rot));
		mrotate.x = World::get().field()->convertCoordToMm(mrotate.x) / CentralAnalyzingUnit::FRAMES_PER_SECOND;
		mrotate.y = World::get().field()->convertCoordToMm(mrotate.y) / CentralAnalyzingUnit::FRAMES_PER_SECOND;

		XBee::out[index].vx = clamp<signed char, -127, 127>(40 * rotated.x);
		XBee::out[index].vy = clamp<signed char, -127, 127>(40 * rotated.y);
		double diff;
		{
			Vector2 cur(robot->orientation());
			Vector2 tgt(rotate);
			diff = cur.angle() - tgt.angle();
			while (diff >= 180)  diff -= 360;
			while (diff <= -180) diff += 360;
			XBee::out[index].vtheta = clamp<signed char, -127, 127>(rotPIDs[index].process(-diff / 180));
		}
		XBee::out[index].dribble    = dribble;
		XBee::out[index].kick       = kick;
		XBee::out[index].emergency  = (RobotController::localEStop || EmergencyStopButton::state) ? 0xFF : 0;
		XBee::out[index].vxMeasured = clamp<signed char, -127, 127>(mrotate.x * 127);
		XBee::out[index].vyMeasured = clamp<signed char, -127, 127>(mrotate.y * 127);

		if (XBee::out[index].emergency)
			rotPIDs[index].clear();

		XBee::update();
	}
}

bool RobotController::localEStop = false;

void RobotController::setSimulation(bool sim) {
	simulation = sim;
}

void RobotController::sendCommand(PPlayer robot, Vector2 acc, double rotate, unsigned char dribble, unsigned char kick) {
	if (simulation)
		simulateWorld(robot, acc, rotate, dribble, kick);
	else
		sendWireless(robot, acc, rotate, dribble, kick);
}

