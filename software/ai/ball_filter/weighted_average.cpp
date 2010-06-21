#include "ai/ball_filter/ball_filter.h"

namespace {
	class weighted_average_filter : public ball_filter {
		public:
			weighted_average_filter() : ball_filter("Weighted Average Filter") {
			}

			point filter(const std::vector<std::pair<double, point> > &balls, friendly_team &, enemy_team &) {
				if (balls.empty()) {
					return point();
				} else {
					point p;
					double total_confidence = 0;
					for (std::vector<std::pair<double, point> >::const_iterator i = balls.begin(); i != balls.end(); ++i) {
						p += i->second * i->first;
						total_confidence += i->first;
					}
					return p / total_confidence;
				}
			}

	} instance;
}

