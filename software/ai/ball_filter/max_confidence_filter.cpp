#include "ai/ball_filter/ball_filter.h"
#include <algorithm>

namespace {
	class MaxConfidenceFilter : public BallFilter {
		public:
			MaxConfidenceFilter() : BallFilter("Max Confidence") {
			}

			Point filter(const std::vector<std::pair<double, Point> > &balls, FriendlyTeam &, EnemyTeam &) {
				return std::max_element(balls.begin(), balls.end())->second;
			}
	};

	MaxConfidenceFilter instance;
}

