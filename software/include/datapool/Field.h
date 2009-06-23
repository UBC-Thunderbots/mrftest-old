#ifndef DATAPOOL_FIELD_H
#define DATAPOOL_FIELD_H

#include <istream>
#include <ostream>
#include "datapool/Vector2.h"

/*
 * A goal.
 */
class Goal {
public:
	/*
	 * Creates a new Goal.
	 */
	Goal();

	/*
	 * North and south edges of the goal.
	 */
	Vector2 north, south;

	/*
	 * North and south edges of defense area.
	 */
	Vector2 defenseN, defenseS;

	/*
	 * Height of goal.
	 */
	unsigned int height;

	Vector2 penalty;

	/*
	 * Compares two goals for equality.
	 */
	bool operator==(const Goal &other) const {
		return north == other.north && south == other.south && defenseN == other.defenseN && defenseS == other.defenseS && height == other.height && penalty == other.penalty;
	}

	/*
	 * Compares two goals for inequality.
	 */
	bool operator!=(const Goal &other) const {
		return !(*this == other);
	}
};

/*
 * Writes a text representation of a goal to a match log.
 */
std::ostream &operator<<(std::ostream &stream, const Goal &goal);

/*
 * Reads a text representation of a goal from a match log.
 */
std::istream &operator>>(std::istream &stream, Goal &goal);



/*
 * The field.
 */
class Field {
public:
	/*
	 * Creates a new field.
	 */
	Field();
	
	/*
	 * Returns information about the field.
	 */
	Goal &eastGoal();
	const Goal &eastGoal() const;
	Goal &westGoal();
	const Goal &westGoal() const;
	int north() const;
	void north(int n);
	int south() const;
	void south(int s);
	int east() const;
	void east(int e);
	int west() const;
	void west(int w);
	Vector2 centerCircle() const;
	void centerCircle(const Vector2 &cc);
	unsigned int centerCircleRadius() const;
	void centerCircleRadius(unsigned int cr);
	int width() const;
	void width(int w);
	int height() const;
	void height(int h);
	double convertMmToCoord(double mm) const;
	double convertCoordToMm(double coord) const;
	Vector2 convertMmToCoord(const Vector2 &mm) const;
	Vector2 convertCoordToMm(const Vector2 &coord) const;
	double infinity() const;
	void infinity(double inf);
	bool isInfinity(double v) const;

	//
	// Compares two fields for equality.
	//
	bool operator==(const Field &other) const {
		return centerCircle_ == other.centerCircle_ && centerCircleRadius_ == other.centerCircleRadius_ && westGoal_ == other.westGoal_ && eastGoal_ == other.eastGoal_ && west_ == other.west_ && east_ == other.east_ && north_ == other.north_ && south_ == other.south_ && width_ == other.width_ && height_ == other.height_ && infinity_ == other.infinity_;
	}

	//
	// Compares two fields for inequality.
	//
	bool operator!=(const Field &other) const {
		return !(*this == other);
	}

private:
	Vector2 centerCircle_;
	unsigned int centerCircleRadius_;
	Goal westGoal_;
	Goal eastGoal_;
	int west_, east_, north_, south_; //pixel coordinates of sidelines distances from X = 0, Y = 0;
	int width_, height_;
	double infinity_;

	friend std::ostream &operator<<(std::ostream &stream, const Field &fld);
	friend std::istream &operator>>(std::istream &stream, Field &fld);
};

/*
 * Writes a text representation of the field to a match log.
 */
std::ostream &operator<<(std::ostream &stream, const Field &fld);

/*
 * Reads a text representation of the field from a match log.
 */
std::istream &operator>>(std::istream &stream, Field &fld);

#endif

