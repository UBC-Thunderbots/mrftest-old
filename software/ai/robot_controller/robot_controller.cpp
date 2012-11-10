#include "ai/robot_controller/robot_controller.h"
#include "util/param.h"

using AI::RC::RobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

void RobotController::convert_to_wheels(const Point &vel, Angle avel, int(&wheel_speeds)[4]) {
	static const double WHEEL_MATRIX[4][3] = {
		{ -42.5995, 27.6645, 4.3175 },
		{ -35.9169, -35.9169, 4.3175 },
		{ 35.9169, -35.9169, 4.3175 },
		{ 42.5995, 27.6645, 4.3175 }
	};
	const double input[3] = { vel.x, vel.y, avel.to_radians() };
	double output[4] = { 0, 0, 0, 0 };
	for (unsigned int row = 0; row < 4; ++row) {
		for (unsigned int col = 0; col < 3; ++col) {
			output[row] += WHEEL_MATRIX[row][col] * input[col];
		}
	}
	for (unsigned int row = 0; row < 4; ++row) {
		wheel_speeds[row] = static_cast<int>(output[row]);
	}
}

RobotController::~RobotController() = default;

void RobotController::draw_overlay(Cairo::RefPtr<Cairo::Context>) {
}

RobotController::RobotController(AI::RC::W::World world, AI::RC::W::Player player) : world(world), player(player) {
}

Gtk::Widget *RobotControllerFactory::ui_controls() {
	return 0;
}

RobotControllerFactory::RobotControllerFactory(const char *name) : Registerable<RobotControllerFactory>(name) {
}

