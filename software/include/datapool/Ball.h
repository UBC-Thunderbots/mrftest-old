#include <tr1/memory>
class Ball;
typedef std::tr1::shared_ptr<Ball> PBall;

#ifndef DATAPOOL_BALL_H
#define DATAPOOL_BALL_H

#include "datapool/PredictableObject.h"

/*
 * The ball.
 */
class Ball : public PredictableObject {
public:
	/*
	 * Creates a new ball.
	 */
	static PBall create();

private:
	Ball();
	Ball(const Ball &copyref); // Prohibit copying.
};

#endif

