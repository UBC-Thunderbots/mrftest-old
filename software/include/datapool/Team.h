class Team;

#ifndef DATAPOOL_TEAM_H
#define DATAPOOL_TEAM_H

#include "datapool/Noncopyable.h"
#include "datapool/Player.h"
#include <cstddef>
#include <vector>

/*
 * A team of robots, which does not itself have any intelligence.
 */
class Team : private virtual Noncopyable {
public:
	/*
	 * The number of robots on a team.
	 */
	static const std::size_t SIZE = 5;

	/*
	 * Constructs a new team. The team's robots will be numbered from 0 to SIZE-1.
	 */
	Team(unsigned int id);

	/*
	 * Runs any updates needed on a per-frame basis. In this class there are none, but subclasses may add some.
	 */
	virtual void update();

	/*
	 * Gets one of the players on the team. Index must range from 0 to SIZE-1.
	 */
	PPlayer player(unsigned int index);
	const PPlayer player(unsigned int index) const;

	/*
	 * Gets all the players on the team.
	 */
	const std::vector<PPlayer> &players();

	/*
	 * Which side the team is on (true=west).
	 */
	bool side() const;
	void side(bool west);

	/*
	 * Whether or not the team has possession of the ball during a special play.
	 */
	bool specialPossession() const;
	void specialPossession(bool special);

	/*
	 * The number of points the team has.
	 */
	unsigned int score() const;
	void score(unsigned int pts);

	/*
	 * The other team.
	 */
	Team &other();
	const Team &other() const;

	//
	// The index of this team (friendly=0, enemy=1).
	//
	unsigned int index() const;

protected:
	std::vector<PPlayer> robots;

private:
	bool west;
	bool special;
	unsigned int points;
	unsigned int id;
	unsigned int activePl;
};

#endif

