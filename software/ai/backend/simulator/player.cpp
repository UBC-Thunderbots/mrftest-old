#include "ai/backend/simulator/player.h"
#include "ai/backend/simulator/backend.h"
#include <algorithm>
#include <cstring>

AI::BE::Simulator::Player::Ptr AI::BE::Simulator::Player::create(Backend &be, unsigned int pattern) {
	Ptr p(new Player(be, pattern));
	return p;
}

void AI::BE::Simulator::Player::pre_tick(const ::Simulator::Proto::S2APlayerInfo &state, const timespec &ts) {
	AI::BE::Simulator::Robot::pre_tick(state.robot_info, ts);
	has_ball_ = state.has_ball;
	kick_ = false;
}

void AI::BE::Simulator::Player::encode_orders(::Simulator::Proto::A2SPlayerInfo &orders) {
	orders.pattern = pattern();
	orders.kick = kick_;
	orders.chip = false;
	orders.chick_power = chick_power_;
	std::copy(&wheel_speeds_[0], &wheel_speeds_[4], &orders.wheel_speeds[0]);
}

void AI::BE::Simulator::Player::mouse_pressed(Point p, unsigned int btn) {
	if (btn == 1 && (p - position()).len() < MAX_RADIUS) {
		disconnect_mouse();
		mouse_connections[0] = be.signal_mouse_released.connect(sigc::mem_fun(this, &Player::mouse_released));
		mouse_connections[1] = be.signal_mouse_exited.connect(sigc::mem_fun(this, &Player::mouse_exited));
		mouse_connections[2] = be.signal_mouse_moved.connect(sigc::mem_fun(this, &Player::mouse_moved));
	}
}

void AI::BE::Simulator::Player::mouse_released(Point, unsigned int btn) {
	if (btn == 1) {
		disconnect_mouse();
	}
}

void AI::BE::Simulator::Player::mouse_exited() {
	disconnect_mouse();
}

void AI::BE::Simulator::Player::mouse_moved(Point p) {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2S_PACKET_DRAG_PLAYER;
	packet.drag.pattern = pattern();
	packet.drag.x = p.x;
	packet.drag.y = p.y;
	be.send_packet(packet);
}

Visualizable::Colour AI::BE::Simulator::Player::visualizer_colour() const {
	return Visualizable::Colour(0.0, 1.0, 0.0);
}

Glib::ustring AI::BE::Simulator::Player::visualizer_label() const {
	return Robot::visualizer_label();
}

bool AI::BE::Simulator::Player::highlight() const {
	return mouse_connections[0].connected();
}

Visualizable::Colour AI::BE::Simulator::Player::highlight_colour() const {
	return Robot::highlight_colour();
}

Point AI::BE::Simulator::Player::position() const {
	return Robot::position();
}

Point AI::BE::Simulator::Player::position(double delta) const {
	return Robot::position(delta);
}

Point AI::BE::Simulator::Player::position(const timespec &ts) const {
	return Robot::position(ts);
}

Point AI::BE::Simulator::Player::velocity(double delta) const {
	return Robot::velocity(delta);
}

Point AI::BE::Simulator::Player::velocity(const timespec &ts) const {
	return Robot::velocity(ts);
}

Point AI::BE::Simulator::Player::acceleration(double delta) const {
	return Robot::acceleration(delta);
}

Point AI::BE::Simulator::Player::acceleration(const timespec &ts) const {
	return Robot::acceleration(ts);
}

double AI::BE::Simulator::Player::orientation() const {
	return Robot::orientation();
}

double AI::BE::Simulator::Player::orientation(double delta) const {
	return Robot::orientation(delta);
}

double AI::BE::Simulator::Player::orientation(const timespec &ts) const {
	return Robot::orientation(ts);
}

double AI::BE::Simulator::Player::avelocity(double delta) const {
	return Robot::avelocity(delta);
}

double AI::BE::Simulator::Player::avelocity(const timespec &ts) const {
	return Robot::avelocity(ts);
}

double AI::BE::Simulator::Player::aacceleration(double delta) const {
	return Robot::aacceleration(delta);
}

double AI::BE::Simulator::Player::aacceleration(const timespec &ts) const {
	return Robot::aacceleration(ts);
}

unsigned int AI::BE::Simulator::Player::pattern() const {
	return Robot::pattern();
}

ObjectStore &AI::BE::Simulator::Player::object_store() const {
	return Robot::object_store();
}

bool AI::BE::Simulator::Player::has_ball() const {
	return has_ball_;
}

bool AI::BE::Simulator::Player::chicker_ready() const {
	return true;
}

void AI::BE::Simulator::Player::move_impl(Point dest, double ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) {
	destination_.first = dest;
	destination_.second = ori;
	target_velocity_ = vel;
	flags_ = flags;
	move_type_ = type;
	move_prio_ = prio;
}

void AI::BE::Simulator::Player::kick_impl(double power) {
	kick_ = true;
	chick_power_ = power;
}

void AI::BE::Simulator::Player::autokick_impl(double power) {
	kick_impl(power);
}

bool AI::BE::Simulator::Player::has_destination() const {
	return true;
}

const std::pair<Point, double> &AI::BE::Simulator::Player::destination() const {
	return destination_;
}

Point AI::BE::Simulator::Player::target_velocity() const {
	return target_velocity_;
}

unsigned int AI::BE::Simulator::Player::flags() const {
	return flags_;
}

AI::Flags::MoveType AI::BE::Simulator::Player::type() const {
	return move_type_;
}

AI::Flags::MovePrio AI::BE::Simulator::Player::prio() const {
	return move_prio_;
}

void AI::BE::Simulator::Player::path_impl(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) {
	path_ = p;
}

bool AI::BE::Simulator::Player::has_path() const {
	return true;
}

const std::vector<std::pair<std::pair<Point, double>, timespec> > &AI::BE::Simulator::Player::path() const {
	return path_;
}

void AI::BE::Simulator::Player::drive(const int(&w)[4]) {
	std::copy(&w[0], &w[4], &wheel_speeds_[0]);
}

const int(&AI::BE::Simulator::Player::wheel_speeds() const)[4] {
	return wheel_speeds_;
}

AI::BE::Simulator::Player::Player(Backend &be, unsigned int pattern) : AI::BE::Simulator::Robot(pattern), be(be), has_ball_(false), flags_(0), move_type_(AI::Flags::MOVE_NORMAL), move_prio_(AI::Flags::PRIO_LOW), kick_(false), chick_power_(0.0) {
	std::fill(&wheel_speeds_[0], &wheel_speeds_[4], 0);
	be.signal_mouse_pressed.connect(sigc::mem_fun(this, &Player::mouse_pressed));
}

AI::BE::Simulator::Player::~Player() {
}

void AI::BE::Simulator::Player::disconnect_mouse() {
	for (std::size_t i = 0; i < G_N_ELEMENTS(mouse_connections); ++i) {
		mouse_connections[i].disconnect();
	}
}

