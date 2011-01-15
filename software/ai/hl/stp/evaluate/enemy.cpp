#include "ai/hl/stp/evaluate/enemy.h"
#include "ai/hl/util.h"

using namespace AI::HL::W;
using AI::HL::STP::Evaluate::EnemyOrdering;
using AI::HL::STP::Evaluate::EnemyRole;

EnemyRole::EnemyRole(EnemyOrdering o, int i) : ordering_(o), index_(i) {
}

