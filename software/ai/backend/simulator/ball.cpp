#include "ai/backend/simulator/ball.h"
#include "ai/backend/simulator/backend.h"
#include "util/param.h"
#include <cmath>
#include <cstring>

namespace {
	DoubleParam ball_decay_constant("Ball Decay Constant", "Backend/Simulator", 99.0, 0.0, 100.0);
}

AI::BE::Simulator::Ball::Ball(Backend &be) : be(be), pred(1.3e-3, 2, ball_decay_constant) {
	be.signal_mouse_pressed.connect(sigc::mem_fun(this, &Ball::mouse_pressed));
}

void AI::BE::Simulator::Ball::pre_tick(const ::Simulator::Proto::S2ABallInfo &state, const timespec &ts) {
	pred.add_measurement(Point(state.x, state.y), ts);
	pred.lock_time(ts);
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

Point AI::BE::Simulator::Ball::position(double delta) const {
	return pred.value(delta).first;
}

Point AI::BE::Simulator::Ball::velocity(double delta) const {
	return pred.value(delta, 1).first;
}

Point AI::BE::Simulator::Ball::position_stdev(double delta) const {
	return pred.value(delta).second;
}

Point AI::BE::Simulator::Ball::velocity_stdev(double delta) const {
	return pred.value(delta, 1).second;
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

