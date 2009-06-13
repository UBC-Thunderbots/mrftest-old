#ifndef TB_WORLD_H
#define TB_WORLD_H

#include "PlayType.h"
#include "Team.h"
#include "Player.h"
#include "Field.h"
#include "Ball.h"

// class to keep track of everything in the world.
class World {
public:
	/*
	 * Initializes the singleton world.
	 */
	static void init(PTeam friendlyTeam, PTeam enemyTeam, PField field);

	/*
	 * Returns the singleton world.
	 */
	static World &get();

	/*
	 * Gets the friendly team.
	 */
	PTeam friendlyTeam();
	const PTeam friendlyTeam() const;

	/*
	 * Gets the enemy team.
	 */
	PTeam enemyTeam();
	const PTeam enemyTeam() const;

	/*
	 * Gets a team by index.
	 */
	PTeam team(unsigned int id);
	const PTeam team(unsigned int id) const;

	/*
	 * Gets the field.
	 */
	PField field();
	const PField field() const;

	/*
	 * Runs the AI.
	 */
	void update();

	/*
	 * What type of play is currently occurring.
	 */
	PlayType::Type playType() const;
	void playType(PlayType::Type type);

	/*
	 * Gets a player.
	 */
	PPlayer player(unsigned int idx);
	const PPlayer player(unsigned int idx) const;

	/*
	 * Gets all the players.
	 */
	const std::vector<PPlayer> &players();

	/*
	 * Gets the ball.
	 */
	PBall ball();
	const PBall ball() const;
	
	bool isBallVisible() const;
	void isBallVisible(bool newVal);

private:
	World(PTeam friendlyTeam, PTeam enemyTeam, PField field);
	World(const World &copyref); // Prohibit copying.

	PlayType::Type play;
	PTeam teams[2];
	PField field_;
	PBall ball_;
	std::vector<PPlayer> everyone;
	bool ballVisible;
};

#endif

