#include <tr1/memory>
class AITeam;
typedef std::tr1::shared_ptr<AITeam> PAITeam;

#ifndef TB_AITEAM_H
#define TB_AITEAM_H

#include "datapool/Team.h"
#include "AI/DecisionUnit.h"
#include "AI/CentralStrategyUnit.h"
#include "AI/LocalStrategyUnit.h"

/*
 * A team that is controlled by the local AI.
 */
class AITeam : public Team {
public:
	/*
	 * Constructs a new AITeam.
	 */
	static PAITeam create(unsigned int id);

	/*
	 * Destroys the team.
	 */
	virtual ~AITeam();

	/*
	 * Runs a frame of AI updates.
	 */
	virtual void update();

	/*
	 * Returns the DecisionUnit.
	 */
	DecisionUnit &getDU();
	const DecisionUnit &getDU() const;

	/*
	 * Returns the CentralStrategyUnit.
	 */
	CentralStrategyUnit &getCSU();
	const CentralStrategyUnit &getCSU() const;

	/*
	 * Returns the LocalStrategyUnit.
	 */
	LocalStrategyUnit &getLSU();
	const LocalStrategyUnit &getLSU() const;

private:
	AITeam(unsigned int id);
	AITeam(const AITeam &copyref); // Prohibit copying.
	DecisionUnit du;
	CentralStrategyUnit csu;
	LocalStrategyUnit lsu;
};

#endif

