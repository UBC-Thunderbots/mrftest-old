#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	/**
	 * Handles both passer and passee class
	 * TODO: finish this.
	 */
	class PassManager : public ByRef {
		public:
			typedef RefPtr<PassManager> Ptr;

			PassManager(Coordinate s, Coordinate t) : passer_dest(s), passee_dest(t) {
			}

			~PassManager() {
			}

			class Passee : public Tactic {
				public:
					Passee(World &world, PassManager::Ptr m) : Tactic(world), manager(m) {
					}
					bool done() const {
						return (player->position() - manager->passee_dest()).len() < AI::HL::Util::POS_CLOSE;
					}
				private:
					PassManager::Ptr manager;
					double score(Player::Ptr player) const {
						return -(player->position() - manager->passee_dest()).lensq();
					}
					void execute() {
						// TODO: fix this movement
						player->move(manager->passee_dest(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_MEDIUM);
					}
			};

			class Passer : public Tactic {
				public:
					Passer(World &world, PassManager::Ptr m) : Tactic(world, true), manager(m) {
					}
					bool done() const {
						return manager->passee->done() && (player->position() - manager->passer_dest()).len() < AI::HL::Util::POS_CLOSE;
					}
				private:
					PassManager::Ptr manager;
					double score(Player::Ptr player) const {
						if (player->has_ball()) return 1.0;
						return 0;
					}
					void execute() {
						// TODO: fix this movement
						player->move(manager->passer_dest(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_MEDIUM);
					}
			};

		protected:
			Coordinate passer_dest;
			Coordinate passee_dest;
			RefPtr<Passer> passer;
			RefPtr<Passee> passee;
	};

	class PasserReady : public Tactic {
		public:
			PasserReady(World &world, Coordinate p, Coordinate t) : Tactic(world), pos(p), target(t) {
			}
		private:
			Coordinate pos, target;
			double score(Player::Ptr player) const {
				if (player->has_ball()) return 1.0;
				return 0;
			}
			void execute() {
				// TODO: fix this movement
				player->move(pos(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_MEDIUM);
				// orient towards target
				player->move(pos(), (target() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_MEDIUM);
				player->kick(7.5);
			}
	};

	class PasseeReady : public Tactic {
		public:
			// ACTIVE tactic!
			PasseeReady(World &world, Coordinate p) : Tactic(world, true), pos(p) {
			}
		private:
			Coordinate pos;
			bool done() const {
				return (player->position() - pos()).len() < AI::HL::Util::POS_CLOSE;
			}
			double score(Player::Ptr player) const {
				return -(player->position() - pos()).lensq();
			}
			void execute() {
				player->move(pos(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_ready(World &world, Coordinate pos, Coordinate target) {
	const Tactic::Ptr p(new PasserReady(world, pos, target));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_ready(World &world, Coordinate pos) {
	const Tactic::Ptr p(new PasseeReady(world, pos));
	return p;
}

