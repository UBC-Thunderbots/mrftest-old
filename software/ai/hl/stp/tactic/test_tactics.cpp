#include <algorithm>

#include "ai/hl/stp/tactic/test_tactics.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/catch.h"
#include "util/dprint.h"
#include "ai/hl/util.h"

namespace Primitives = AI::BE::Primitives;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
    
	class MoveTest final : public Tactic {
		public:
			explicit MoveTest(World world, Point dest) : Tactic(world), dest(dest) {
			}

		private:
			const Point dest;
            Point original_pos;
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"move test";
			}
	};

	Player MoveTest::select(const std::set<Player> &players) const {
		Player p = *(players.begin());
		return p;
	}

	void MoveTest::execute(caller_t& ca) {
        original_pos = player().position();
		AI::HL::STP::Action::move_slp(ca, world, player(), dest);
        AI::HL::STP::Action::move_slp(ca, world, player(), original_pos);
	}

	class MoveTestOrientation final : public Tactic {
		public:
			explicit MoveTestOrientation(World world, Point dest) : Tactic(world), dest(dest) {
			}

		private:
			const Point dest;
            Point original_pos;
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"move test";
			}
	};

	Player MoveTestOrientation::select(const std::set<Player> &players) const {
		Player p = *(players.begin());
		return p;
	}

	void MoveTestOrientation::execute(caller_t& ca) {
		while(true) {
			AI::HL::STP::Action::pivot(ca, world, player(), Point(), Angle::zero(), 1);
			yield(ca);
		}




//		// testing if rrt mvoes out of the field
//		AI::Flags::MoveFlags flags = player().flags();
//		if ((flags & AI::Flags::MoveFlags::CLIP_PLAY_AREA) != AI::Flags::MoveFlags::NONE) {
//			LOG_INFO(u8"CLIP PLAY AREA FLAG SET");
//		}else {
//			LOG_INFO(u8"FLAAG NOT Set");
//		}
//
////		LOG_INFO(u8"First Point");
//		Point p = Point(0, 0);
////		while((player().position() - p).len() > 0.05) {
////			AI::HL::STP::Action::move_rrt(ca, world, player(), p);
////			yield(ca);
////		}
//		LOG_INFO(u8"Second Point");
//		p = Point(0, world.field().width() / 2 + 0.15);
//		while((player().position() - p).len() > 0.05) {
//			AI::HL::STP::Action::move_rrt(ca, world, player(), p);
//			yield(ca);
//		}
//		LOG_INFO(u8"Third Point");
//		p = Point(world.field().length()/2, world.field().width() / 2);
//		while((player().position() - p).len() > 0.05) {
//			AI::HL::STP::Action::move_rrt(ca, world, player(), p);
//			yield(ca);
//		}

//        original_pos = player().position();
//		AI::HL::STP::Action::move_slp(ca, world, player(), dest);
//		AI::HL::STP::Action::move_slp(ca, world, player(), original_pos);
	}

	class ShootTest final : public Tactic {
		public:
			explicit ShootTest(World world) : Tactic(world) {
			}

		private:
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& caller) override;

			Glib::ustring description() const override {
				return u8"shoot test";
			}
	};

	Player ShootTest::select(const std::set<Player> &players) const {
		Player p = *(players.begin());
		return p;
	}

	void ShootTest::execute(caller_t& ca) {
		AI::HL::STP::Action::catch_and_shoot_goal(ca, world, player(), false, true);
	}

	class CatchTest final : public Tactic {
        public:
            explicit CatchTest(World world) : Tactic(world) {
            }

        private:
            Player select(const std::set<Player> &players) const override;
            void execute(caller_t& caller) override;

            Glib::ustring description() const override {
                return u8"catch test";
            }
    };

    Player CatchTest::select(const std::set<Player> &players) const {
        Player p = *(players.begin());
        return p;
    }

    void CatchTest::execute(caller_t& ca) {
        AI::HL::STP::Action::catch_ball(ca, world, player(), world.field().enemy_goal());
    }

	class JCatchTest final : public Tactic {
        public:
            explicit JCatchTest(World world) : Tactic(world) {
            }

        private:
            Player select(const std::set<Player> &players) const override;
            void execute(caller_t& caller) override;

            Glib::ustring description() const override {
                return u8"catch test";
            }
    };

    Player JCatchTest::select(const std::set<Player> &players) const {
        Player p = *(players.begin());
        return p;
    }

    void JCatchTest::execute(caller_t& ca) {
        AI::HL::STP::Action::just_catch_ball(ca, world, player());
    }
}



Tactic::Ptr AI::HL::STP::Tactic::move_test(World world, Point dest) {
	return Tactic::Ptr(new MoveTest(world, dest));
}

Tactic::Ptr AI::HL::STP::Tactic::move_test_orientation(World world, Point dest) {
	return Tactic::Ptr(new MoveTestOrientation(world, dest));
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_test(World world) {
	return Tactic::Ptr(new ShootTest(world));
}

Tactic::Ptr AI::HL::STP::Tactic::catch_test(World world) {
    LOGF_INFO(u8"%1", "Catching");
	return Tactic::Ptr(new CatchTest(world));
}

Tactic::Ptr AI::HL::STP::Tactic::just_catch_test(World world) {
	return Tactic::Ptr(new JCatchTest(world));
}
