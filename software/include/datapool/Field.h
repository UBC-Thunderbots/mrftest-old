#include <tr1/memory>
class Goal;
typedef std::tr1::shared_ptr<Goal> PGoal;
class Field;
typedef std::tr1::shared_ptr<Field> PField;

#ifndef DATAPOOL_FIELD_H
#define DATAPOOL_FIELD_H

#include "datapool/Vector2.h"

/*
 * A goal.
 */
class Goal {
public:
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
	 * Creates a new Goal object.
	 */
	static PGoal create(const Vector2 &north, const Vector2 &south, const Vector2 &defenseN, const Vector2 &defenseS, unsigned int height, const Vector2 &penalty);

private:
	Goal(const Vector2 &north, const Vector2 &south, const Vector2 &defenseN, const Vector2 &defenseS, unsigned int height, const Vector2 &penalty);
};



/*
 * The field.
 */
class Field {
public:
	/*
	 * Creates a new field.
	 */
	static PField create(int width, int height, int west, int east, int north, int south, const Vector2 &centerCircle, unsigned int centerCircleRadius, PGoal westGoal, PGoal eastGoal);
	
	/*
	 * Returns information about the field.
	 */
	PGoal eastGoal();
	const PGoal eastGoal() const;
	PGoal westGoal();
	const PGoal westGoal() const;
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

private:
	Field(int width, int height, int west, int east, int north, int south, const Vector2 &centerCircle, unsigned int centerCircleRadius, PGoal westGoal, PGoal eastGoal);
	Field(const Field &copyref); // Prohibit copying.

	Vector2 centerCircle_;
	unsigned int centerCircleRadius_;
	PGoal westGoal_;
	PGoal eastGoal_;
	int west_, east_, north_, south_; //pixel coordinates of sidelines distances from X = 0, Y = 0;
	int width_, height_;
};

#endif

