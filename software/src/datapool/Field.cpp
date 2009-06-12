#include "datapool/Field.h"



PGoal Goal::create(const Vector2 &north, const Vector2 &south, const Vector2 &defenseN, const Vector2 &defenseS, unsigned int height, const Vector2 &penalty) {
	PGoal g(new Goal(north, south, defenseN, defenseS, height, penalty));
	return g;
}

Goal::Goal(const Vector2 &north, const Vector2 &south, const Vector2 &defenseN, const Vector2 &defenseS, unsigned int height, const Vector2 &penalty) : north(north), south(south), defenseN(defenseN), defenseS(defenseS), height(height), penalty(penalty) {
}



PField Field::create(int width, int height, int west, int east, int north, int south, const Vector2 &centerCircle, unsigned int centerCircleRadius, PGoal westGoal, PGoal eastGoal) {
	PField field(new Field(width, height, west, east, north, south, centerCircle, centerCircleRadius, westGoal, eastGoal));
	return field;
}

Field::Field(int width, int height, int west, int east, int north, int south, const Vector2 &centerCircle, unsigned int centerCircleRadius, PGoal westGoal, PGoal eastGoal) : centerCircle_(centerCircle), centerCircleRadius_(centerCircleRadius), westGoal_(westGoal), eastGoal_(eastGoal), west_(west), east_(east), north_(north), south_(south), width_(width), height_(height) {
}

Vector2 Field::centerCircle() const {
	return centerCircle_;
}

void Field::centerCircle(const Vector2 &cc) {
	centerCircle_ = cc;
}

unsigned int Field::centerCircleRadius() const {
	return centerCircleRadius_;
}

void Field::centerCircleRadius(unsigned int cr) {
	centerCircleRadius_ = cr;
}

int Field::width() const {
	return width_;
}

void Field::width(int w) {
	width_ = w;
}

int Field::height() const {
	return height_;
}

void Field::height(int h) {
	height_ = h;
}

PGoal Field::westGoal() {
	return westGoal_;
}

const PGoal Field::westGoal() const {
	return westGoal_;
}

PGoal Field::eastGoal() {
	return eastGoal_;
}

const PGoal Field::eastGoal() const {
	return eastGoal_;
}

int Field::north() const {
	return north_;
}

void Field::north(int n) {
	north_ = n;
}

int Field::south() const {
	return south_;
}

void Field::south(int s) {
	south_ = s;
}

int Field::east() const {
	return east_;
}

void Field::east(int e) {
	east_ = e;
}

int Field::west() const {
	return west_;
}

void Field::west(int w) {
	west_ = w;
}

double Field::convertMmToCoord (double mm) const {
	// Use the official width of the field as a conversion factor.
	return (mm * width()) / 6050.0;
}

double Field::convertCoordToMm(double coord) const {
	return coord * 6050.0 / width();
}

