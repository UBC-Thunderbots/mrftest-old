#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"

#include <map>

using namespace AI::HL::STP;
using namespace AI::HL::W;
using AI::HL::STP::Predicates::Predicate;

namespace {
	template<typename cmp> struct BallX : public Predicate {
		double x;
		bool evaluate(World& world) const {
			// TODO: this does not look right
			return cmp()(world.ball().position().x, x);
		}
	};

	struct Negate : public Predicate {
		const Predicate* predicate;
		bool evaluate(World& world) const {
			return !predicate->evaluate(world);
		}
	};
}

const Predicate* AI::HL::STP::Predicates::negate(const Predicate* predicate) {
	static std::map<const Predicate*, Negate> conds;
	conds[predicate].predicate = predicate;
	// TODO: have to ensure that negation of negation should not be valid
	return &conds[predicate];
}

const Predicate* AI::HL::STP::Predicates::goal() {
	struct Cond : public Predicate {
		bool evaluate(World&) const {
			return false;
		}
	};
	static Cond cond;
	return &cond;
}

const Predicate* AI::HL::STP::Predicates::playtype(const PlayType::PlayType playtype) {
	struct Cond : public Predicate {
		PlayType::PlayType playtype;
		bool evaluate(World& world) const {
			return world.playtype() == playtype;
		}
	};
	static Cond conds[PlayType::COUNT];
	conds[playtype].playtype = playtype;
	return &conds[playtype];
}

const Predicate* AI::HL::STP::Predicates::our_ball() {
	struct Cond : public Predicate {
		bool evaluate(World& world) const {
			FriendlyTeam& friendly = world.friendly_team();
			for (std::size_t i = 0; i < friendly.size(); ++i) {
				if (friendly.get(i)->has_ball()) {
					return true;
				}
			}
			return false;
		}
	};
	static Cond cond;
	return &cond;
}

const Predicate* AI::HL::STP::Predicates::their_ball() {
	struct Cond : public Predicate {
		bool evaluate(World& world) const {
			EnemyTeam& enemy = world.enemy_team();
			for (std::size_t i = 0; i < enemy.size(); ++i) {
				if (AI::HL::Util::posses_ball(world, enemy.get(i))) {
					return true;
				}
			}
			return false;
		}
	};
	static Cond cond;
	return &cond;
}

const Predicate* AI::HL::STP::Predicates::none_ball() {
	struct Cond : public Predicate {
		bool evaluate(World& world) const {
			FriendlyTeam& friendly = world.friendly_team();
			for (std::size_t i = 0; i < friendly.size(); ++i) {
				if (friendly.get(i)->has_ball()) {
					return false;
				}
			}
			EnemyTeam& enemy = world.enemy_team();
			for (std::size_t i = 0; i < enemy.size(); ++i) {
				if (AI::HL::Util::posses_ball(world, enemy.get(i))) {
					return true;
				}
			}

			return true;
		}
	};
	static Cond cond;
	return &cond;
}

const Predicate* AI::HL::STP::Predicates::our_team_size_at_least(const unsigned int n) {
	struct Cond : public Predicate {
		unsigned int n;
		bool evaluate(World& world) const {
			return world.friendly_team().size() >= n;
		}
	};
	static Cond conds[6];
	conds[n].n = n;
	return &conds[n];
}

const Predicate* AI::HL::STP::Predicates::their_team_size_at_least(const unsigned int n) {
	struct Cond : public Predicate {
		unsigned int n;
		bool evaluate(World& world) const {
			return world.enemy_team().size() >= n;
		}
	};
	static Cond conds[6];
	conds[n].n = n;
	return &conds[n];
}

const Predicate* AI::HL::STP::Predicates::their_team_size_at_most(const unsigned int n) {
	struct Cond : public Predicate {
		unsigned int n;
		bool evaluate(World& world) const {
			return world.enemy_team().size() <= n;
		}
	};
	static Cond conds[6];
	conds[n].n = n;
	return &conds[n];
}

const Predicate* AI::HL::STP::Predicates::ball_x_less_than(const double x) {
	static std::map<double, BallX<std::less<double> > > conds;
	conds[x].x = x;
	return &conds[x];
}

const Predicate* AI::HL::STP::Predicates::ball_x_greater_than(const double x) {
	static std::map<double, BallX<std::greater<double> > > conds;
	conds[x].x = x;
	return &conds[x];
}

const Predicate* AI::HL::STP::Predicates::ball_on_our_side() {
	struct Cond : public Predicate {
		bool evaluate(World& world) const {
			return world.ball().position().x <= 0;
		}
	};
	static Cond cond;
	return &cond;
}

const Predicate* AI::HL::STP::Predicates::ball_on_their_side() {
	struct Cond : public Predicate {
		bool evaluate(World& world) const {
			return world.ball().position().x > 0;
		}
	};
	static Cond cond;
	return &cond;
}

