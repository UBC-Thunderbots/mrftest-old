#include <tr1/memory>
class Player;
typedef std::tr1::shared_ptr<Player> PPlayer;

#ifndef TB_PLAYER_H
#define TB_PLAYER_H

#include "datapool/PredictableObject.h"
#include "datapool/Plan.h"
#include "datapool/Team.h"

#define DEFAULT_MAX_ACC    2

class Player : public PredictableObject {
public:
	/*
	 * Creates a new player.
	 */
	static PPlayer create(PTeam team);

	/*
	 * Destroys a player.
	 */
	virtual ~Player();

	/*
	 * Gets or sets the current plan.
	 */
	Plan::Behavior plan() const;
	void plan(Plan::Behavior plan);

	/*
	 * Gets or sets the current orientation (observed, not desired!)
	 */
	double orientation() const;
	void orientation(double o);

	/*
	 * Checks or sets whether the player has the ball.
	 */
	bool hasBall() const;
	void hasBall(bool has);

	/*
	 * Checks or sets whether the player is receiving a pass.
	 */
	bool receivingPass() const;
	void receivingPass(bool receiving);
	
	/*
	 * Sets whether the robot is allowed inside the ball's radius during special plays
	 */
	bool allowedInside() const;
	void allowedInside(bool val);

	/*
	 * Gets the team the player belongs to.
	 */
	PTeam team();
	const PTeam team() const;

	/*
	 * The player this player is passing to or guarding.
	 */
	const PPlayer otherPlayer() const;
	PPlayer otherPlayer();
	void otherPlayer(const PPlayer &other);

	/*
	 * The location this player is trying to move to.
	 */
	const Vector2 &destination() const;
	void destination(const Vector2 &dest);

	/*
	 * The most recently requested velocity.
	 */
	const Vector2 &requestedVelocity() const;
	void requestedVelocity(const Vector2 &rv);

private:
	Player(PTeam team);
	Player(const Player &copyref); // Prohibit copying.

	double orient;           //orientation double from 0-360
	bool possession;         //true: has the ball, false: doesn't have the ball
	Plan::Behavior behavior; //used to store robot behavior in CSU.
	bool receiving;          //true if the robot is currently receiving a pass.
	bool allowedIn;		 	 //true if the robot is allowed inside the ball's radius during special plays.
	PTeam tm;                //which team the player is on
	PPlayer other;           //which player I'm passing to or guarding
	Vector2 dest;            //where I'm trying to get to
	Vector2 reqVelocity;     //the last requested velocity
};

#endif

