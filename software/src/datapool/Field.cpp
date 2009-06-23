#include "datapool/Field.h"

Goal::Goal() : height(0) {
}

std::ostream &operator<<(std::ostream &stream, const Goal &goal) {
	return stream << goal.north << ' ' << goal.south << ' ' << goal.defenseN << ' ' << goal.defenseS << ' ' << goal.height << ' ' << goal.penalty;
}

std::istream &operator>>(std::istream &stream, Goal &goal) {
	return stream >> goal.north >> goal.south >> goal.defenseN >> goal.defenseS >> goal.height >> goal.penalty;
}



Field::Field() : centerCircleRadius_(0), west_(0), east_(0), north_(0), south_(0), width_(0), height_(0), infinity_(0) {
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

Goal &Field::westGoal() {
	return westGoal_;
}

const Goal &Field::westGoal() const {
	return westGoal_;
}

Goal &Field::eastGoal() {
	return eastGoal_;
}

const Goal &Field::eastGoal() const {
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

double Field::convertMmToCoord(double mm) const {
	// Use the official width of the field as a conversion factor.
	return (mm * width()) / 6050.0;
}

double Field::convertCoordToMm(double coord) const {
	return coord * 6050.0 / width();
}

Vector2 Field::convertMmToCoord(const Vector2 &mm) const {
	return Vector2(convertMmToCoord(mm.x), convertMmToCoord(mm.y));
}

Vector2 Field::convertCoordToMm(const Vector2 &coord) const {
	return Vector2(convertCoordToMm(coord.x), convertCoordToMm(coord.y));
}

double Field::infinity() const {
	return infinity_;
}

void Field::infinity(double inf) {
	infinity_ = inf;
}

bool Field::isInfinity(double v) const {
	return v >= 0.99 * infinity_;
}

std::ostream &operator<<(std::ostream &stream, const Field &fld) {
	return stream << fld.centerCircle_ << ' ' << fld.centerCircleRadius_ << ' ' << fld.westGoal_ << ' ' << fld.eastGoal_ << ' ' << fld.west_ << ' ' << fld.east_ << ' ' << fld.north_ << ' ' << fld.south_ << ' ' << fld.width_ << ' ' << fld.height_ << ' ' << fld.infinity_;
}

std::istream &operator>>(std::istream &stream, Field &fld) {
	return stream >> fld.centerCircle_ >> fld.centerCircleRadius_ >> fld.westGoal_ >> fld.eastGoal_ >> fld.west_ >> fld.east_ >> fld.north_ >> fld.south_ >> fld.width_ >> fld.height_ >> fld.infinity_;
}

