#include "ai/hl/hl.h"
#include "ai/hl/stp/play_executor.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class STPHLFactory : public HighLevelFactory {
		public:
			STPHLFactory() : HighLevelFactory("STP") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	STPHLFactory factory_instance;

	class STPHL : public HighLevel {
		public:
			STPHL(World &world) : executor(world) {
			}

			STPHLFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				executor.tick();
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			PlayExecutor executor;
	};

	HighLevel::Ptr STPHLFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new STPHL(world));
		return p;
	}
}

