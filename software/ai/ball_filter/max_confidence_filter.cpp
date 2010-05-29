#include "ai/ball_filter/ball_filter.h"

namespace {
	class max_confidence_filter : public ball_filter {
		public:
			max_confidence_filter() : ball_filter("Max Confidence") {
			}

			point filter(const std::vector<std::pair<point, double> > &balls) {
				point best;
				double bestconf = 0.0;
				for (std::vector<std::pair<point, double> >::const_iterator i = balls.begin(), iend = balls.end(); i != iend; ++i) {
					if (i->second > bestconf) {
						best = i->first;
						bestconf = i->second;
					}
				}
				return best;
			}
	};

	max_confidence_filter instance;
}

