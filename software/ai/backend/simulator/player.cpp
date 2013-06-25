#include "ai/backend/simulator/player.h"
#include "ai/backend/simulator/backend.h"
#include <algorithm>
#include <cstring>

AI::BE::Simulator::Player::Player(Backend &be, unsigned int pattern) : AI::BE::Player(pattern), be(be), has_ball_(false), kick_(false), chick_power_(0.0), autokick_fired_(false), autokick_pre_fired_(false), dragging_(false) {
}

void AI::BE::Simulator::Player::pre_tick(const ::Simulator::Proto::S2APlayerInfo &state, const timespec &ts) {
	add_field_data({state.robot_info.x, state.robot_info.y}, Angle::of_radians(state.robot_info.orientation), ts);
	AI::BE::Player::pre_tick();
	AI::BE::Player::lock_time(ts);
	has_ball_ = state.has_ball;
	kick_ = false;
	autokick_fired_ = autokick_pre_fired_;
	autokick_pre_fired_ = false;
}

void AI::BE::Simulator::Player::encode_orders(::Simulator::Proto::A2SPlayerInfo &orders) {
	orders.pattern = pattern();
	orders.kick = kick_;
	orders.chip = false;
	orders.chick_power = chick_power_;
	std::copy(&wheel_speeds_[0], &wheel_speeds_[4], &orders.wheel_speeds[0]);
}

bool AI::BE::Simulator::Player::highlight() const {
	return AI::BE::Player::highlight() || dragging_;
}

void AI::BE::Simulator::Player::dribble_slow() {
}

bool AI::BE::Simulator::Player::has_ball() const {
	return has_ball_;
}

bool AI::BE::Simulator::Player::chicker_ready() const {
	return true;
}

void AI::BE::Simulator::Player::kick_impl(double speed) {
	kick_ = true;
	chick_power_ = speed;
}

void AI::BE::Simulator::Player::autokick_impl(double speed) {
	kick_impl(speed);
	autokick_pre_fired_ = true;
}

bool AI::BE::Simulator::Player::autokick_fired() const {
	return autokick_fired_;
}

