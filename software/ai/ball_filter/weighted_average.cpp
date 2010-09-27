#include "ai/ball_filter/ball_filter.h"

using AI::BF::BallFilter;
using namespace AI::BF::W;

namespace {
	class WeightedAverageFilter : public BallFilter {
		public:
			WeightedAverageFilter() : BallFilter("Weighted Average Filter") {
			}

			Point filter(const std::vector<std::pair<double, Point> > &balls, World &) {
				if (balls.empty()) {
					return Point();
				} else {
					Point p;
					double total_confidence = 0;
					for (std::vector<std::pair<double, Point> >::const_iterator i = balls.begin(); i != balls.end(); ++i) {
						p += i->second * i->first;
						total_confidence += i->first;
					}
					return p / total_confidence;
				}
			}
	} instance;
}

