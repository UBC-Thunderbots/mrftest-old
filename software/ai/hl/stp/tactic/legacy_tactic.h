#pragma once
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				class LegacyTactic : public Tactic {
					public:
						using Ptr = std::unique_ptr<LegacyTactic>;
						explicit LegacyTactic(World world, bool = false) : Tactic(world) { }
						virtual void execute() = 0;
					protected:
						Player player;

						inline void execute(caller_t& caller) override {
							while (true) {
								player = Tactic::player();
								execute();
								caller();
							}
						}
				};
			}
		}
	}
}
