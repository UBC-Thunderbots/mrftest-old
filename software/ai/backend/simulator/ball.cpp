#include "ai/backend/simulator/ball.h"
#include "ai/backend/simulator/backend.h"
#include <cmath>
#include <cstring>

AI::BE::Simulator::Ball::Ball(Backend &be) : be(be), xpred(false), ypred(false) {
	be.signal_mouse_pressed.connect(sigc::mem_fun(this, &Ball::mouse_pressed));
}

AI::BE::Simulator::Ball::~Ball() {
}

void AI::BE::Simulator::Ball::pre_tick(const ::Simulator::Proto::S2ABallInfo &state, const timespec &ts) {
	xpred.add_datum(state.x, ts);
	xpred.lock_time(ts);
	ypred.add_datum(state.y, ts);
	ypred.lock_time(ts);
}

void AI::BE::Simulator::Ball::mouse_pressed(Point p, unsigned int btn) {
	if (btn == 1 && (p - position()).len() < RADIUS) {
		disconnect_mouse();
		mouse_connections[0] = be.signal_mouse_released.connect(sigc::mem_fun(this, &Ball::mouse_released));
		mouse_connections[1] = be.signal_mouse_exited.connect(sigc::mem_fun(this, &Ball::mouse_exited));
		mouse_connections[2] = be.signal_mouse_moved.connect(sigc::mem_fun(this, &Ball::mouse_moved));
	}
}

void AI::BE::Simulator::Ball::mouse_released(Point, unsigned int btn) {
	if (btn == 1) {
		disconnect_mouse();
	}
}

void AI::BE::Simulator::Ball::mouse_exited() {
	disconnect_mouse();
}

void AI::BE::Simulator::Ball::mouse_moved(Point p) {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::DRAG_BALL;
	packet.drag.pattern = 0;
	packet.drag.x = p.x;
	packet.drag.y = p.y;
	be.send_packet(packet);
}

Point AI::BE::Simulator::Ball::position() const {
	return Point(xpred.value(), ypred.value());
}

Point AI::BE::Simulator::Ball::position(double delta) const {
	return Point(xpred.value(delta), ypred.value(delta));
}

Point AI::BE::Simulator::Ball::position(const timespec &ts) const {
	return Point(xpred.value(ts), ypred.value(ts));
}

Point AI::BE::Simulator::Ball::velocity() const {
	return Point(xpred.value(0.0, 1), ypred.value(0.0, 1));
}

Point AI::BE::Simulator::Ball::velocity(double delta) const {
	return Point(xpred.value(delta, 1), ypred.value(delta, 1));
}

Point AI::BE::Simulator::Ball::velocity(const timespec &ts) const {
	return Point(xpred.value(ts, 1), ypred.value(ts, 1));
}

Point AI::BE::Simulator::Ball::acceleration(double delta) const {
	return Point(xpred.value(delta, 2), ypred.value(delta, 2));
}

Point AI::BE::Simulator::Ball::acceleration(const timespec &ts) const {
	return Point(xpred.value(ts, 2), ypred.value(ts, 2));
}

bool AI::BE::Simulator::Ball::highlight() const {
	return mouse_connections[0].connected();
}

Visualizable::Colour AI::BE::Simulator::Ball::highlight_colour() const {
	return Visualizable::Colour(0.0, 0.0, 0.0);
}

void AI::BE::Simulator::Ball::disconnect_mouse() {
	for (std::size_t i = 0; i < G_N_ELEMENTS(mouse_connections); ++i) {
		mouse_connections[i].disconnect();
	}
}

