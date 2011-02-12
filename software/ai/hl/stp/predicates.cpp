#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"

#include <map>

using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	template<typename cmp> struct BallX : public Predicate {
		double x;
		bool evaluate(World& world) const {
			// TODO: this does not look right
			return cmp()(world.ball().position().x, x);
		}
	};
}

const Predicate* AI::HL::STP::playtype(const PlayType::PlayType playtype) {
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

const Predicate* AI::HL::STP::our_ball() {
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

const Predicate* AI::HL::STP::their_ball() {
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

const Predicate* AI::HL::STP::our_team_size(const unsigned int n) {
	struct Cond : public Predicate {
		unsigned int n;
		bool evaluate(World& world) const {
			return world.friendly_team().size() == n;
		}
	};
	static Cond conds[6];
	conds[n].n = n;
	return &conds[n];
}

const Predicate* AI::HL::STP::their_team_size(const unsigned int n) {
	struct Cond : public Predicate {
		unsigned int n;
		bool evaluate(World& world) const {
			return world.enemy_team().size() == n;
		}
	};
	static Cond conds[6];
	conds[n].n = n;
	return &conds[n];
}

const Predicate* AI::HL::STP::ball_x_less_than(const double x) {
	static std::map<double, BallX<std::less<double> > > conds;
	conds[x].x = x;
	return &conds[x];
}

const Predicate* AI::HL::STP::ball_x_greater_than(const double x) {
	static std::map<double, BallX<std::greater<double> > > conds;
	conds[x].x = x;
	return &conds[x];
}

const Predicate* AI::HL::STP::ball_on_our_side() {
	struct Cond : public Predicate {
		bool evaluate(World& world) const {
			return world.ball().position().x <= 0;
		}
	};
	static Cond cond;
	return &cond;
}

const Predicate* AI::HL::STP::ball_on_their_side() {
	struct Cond : public Predicate {
		bool evaluate(World& world) const {
			return world.ball().position().x > 0;
		}
	};
	static Cond cond;
	return &cond;
}

