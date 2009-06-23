#ifndef DATAPOOL_WORLD_H
#define DATAPOOL_WORLD_H

#include "datapool/Ball.h"
#include "datapool/Field.h"
#include "datapool/Noncopyable.h"
#include "datapool/Player.h"
#include "datapool/PlayType.h"
#include "datapool/Team.h"
#include "datapool/Updateable.h"

// class to keep track of everything in the world.
class World : private virtual Noncopyable, public virtual Updateable {
public:
	/*
	 * Initializes the singleton world.
	 */
	static void init(Team &friendlyTeam, Team &enemyTeam, const Field &field);

	/*
	 * Returns the singleton world.
	 */
	static World &get();

	/*
	 * Gets the friendly team.
	 */
	Team &friendlyTeam();
	const Team &friendlyTeam() const;

	/*
	 * Gets the enemy team.
	 */
	Team &enemyTeam();
	const Team &enemyTeam() const;

	/*
	 * Gets a team by index.
	 */
	Team &team(unsigned int id);
	const Team &team(unsigned int id) const;

	/*
	 * Gets the field.
	 */
	Field &field();
	const Field &field() const;

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
	Ball &ball();
	const Ball &ball() const;
	
	//
	// Whether or not the ball is visible on the cameras.
	//
	bool isBallVisible() const;
	void isBallVisible(bool newVal);

private:
	World(Team &friendlyTeam, Team &enemyTeam, const Field &field);

	PlayType::Type play;
	Team &friendly;
	Team &enemy;
	Field field_;
	Ball ball_;
	std::vector<PPlayer> everyone;
	bool ballVisible;
};

#endif

