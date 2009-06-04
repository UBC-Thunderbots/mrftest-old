#include <tr1/memory>
class Team;
typedef std::tr1::shared_ptr<Team> PTeam;

#ifndef TB_TEAM_H
#define TB_TEAM_H

#include "Player.h"
#include <cstddef>
#include <vector>

/*
 * A team of robots, which does not itself have any intelligence.
 */
class Team {
public:
	/*
	 * The number of robots on a team.
	 */
	static const std::size_t SIZE = 5;

	/*
	 * Constructs a new team. The team's robots will be numbered from 0 to SIZE-1.
	 */
	static PTeam create(unsigned int id);

	/*
	 * Destroys the team.
	 */
	virtual ~Team();

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
	PTeam other();
	const PTeam other() const;

protected:
	Team(unsigned int id);
	std::vector<PPlayer> robots;

private:
	Team(const Team &copyref); // Prohibit copying.
	bool west;
	bool special;
	unsigned int points;
	unsigned int id;
};

#endif

