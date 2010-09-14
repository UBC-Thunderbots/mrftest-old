#include "ai/ball_filter/ball_filter.h"
#include <algorithm>

using AI::BF::BallFilter;
using namespace AI::BF::W;

namespace {
	class MaxConfidenceFilter : public BallFilter {
		public:
			MaxConfidenceFilter() : BallFilter("Max Confidence") {
			}

			Point filter(const std::vector<std::pair<double, Point> > &balls, World &) {
				return std::max_element(balls.begin(), balls.end())->second;
			}
	};

	MaxConfidenceFilter instance;
}

