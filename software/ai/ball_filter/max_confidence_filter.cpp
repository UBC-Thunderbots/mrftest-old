#include "ai/ball_filter/ball_filter.h"
#include <algorithm>

namespace {
	class max_confidence_filter : public ball_filter {
		public:
			max_confidence_filter() : ball_filter("Max Confidence") {
			}

			point filter(const std::vector<std::pair<double, point> > &balls, friendly_team &, enemy_team &) {
				return std::max_element(balls.begin(), balls.end())->second;
			}
	};

	max_confidence_filter instance;
}

