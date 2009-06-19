class AITeam;

#ifndef AI_AITEAM_H
#define AI_AITEAM_H

#include "datapool/Noncopyable.h"
#include "datapool/Team.h"
#include "AI/DecisionUnit.h"
#include "AI/CentralStrategyUnit.h"
#include "AI/LocalStrategyUnit.h"

/*
 * A team that is controlled by the local AI.
 */
class AITeam : public Team, private virtual Noncopyable {
public:
	/*
	 * Constructs a new AITeam.
	 */
	AITeam(unsigned int id);

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
	DecisionUnit du;
	CentralStrategyUnit csu;
	LocalStrategyUnit lsu;
};

#endif

