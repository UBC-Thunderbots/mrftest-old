#include "datapool/Object.h"
#include <cassert>

Object::Object() : r(0), pos(0, 0), vel(0, 0), acc(0, 0) {
}

Object::~Object() {
}

double Object::radius() const {
	return r;
}

void Object::radius(double r) {
	this->r = r;
}

const Vector2 &Object::position() const {
	return pos;
}

void Object::position(const Vector2 &p) {
	pos = p;
}

const Vector2 &Object::velocity() const {
	return vel;
}

void Object::velocity(const Vector2 &v) {
	vel = v;
}

const Vector2 &Object::acceleration() const {
	return acc;
}

void Object::acceleration(const Vector2 &a) {
	acc = a;
}

