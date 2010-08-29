#include "ai/ball_filter/ball_filter.h"
#include "util/timestep.h"
#include <cmath>
#include <list>
#include <utility>
#include <vector>

using namespace AI;
using namespace std;

namespace {
	struct Circle {
		Point center;
		double certainty;

		bool operator==(Circle other) {
			return (center - other.center).len() < 0.001;
		}
	};

	class ByronsFilter : public BallFilter {
		private:
			static const double RADIUS = 10.0/TIMESTEPS_PER_SECOND;
			static const double DECAY_RATE = 0.2063; // half-life = 3 frames
			static const double DELETE_THRESHOLD = 0.02; // stores < 50 circles
			list<Circle> circles;
			Point last_point;

		public:
			ByronsFilter() : BallFilter("Byron's Filter") {
				Circle c;
				c.center = Point(0, 0);
				c.certainty = DELETE_THRESHOLD;
				circles.push_back(c);
			}

			Point filter(const vector<pair<double, Point> > &obs, FriendlyTeam &, EnemyTeam &) {
				Point max_point;
				double max_cert = -0.1;

				if (obs.empty()) {
					for (list<Circle>::iterator it = circles.begin(); it != circles.end(); ++it) {
						it->certainty = (1.0 - DECAY_RATE)*it->certainty;
					}
				}
				else {
					for (unsigned int i = 0; i < obs.size(); i++) {
						if (max_cert < obs[i].first) {
							max_point = obs[i].second;
							max_cert = obs[i].first;
						}
					}

					vector<Circle> containing;
					for (list<Circle>::iterator it = circles.begin(); it != circles.end(); ++it) {
						if ((max_point - it->center).len() < RADIUS) {
							containing.push_back(*it);
							it->center = max_point;
						}
						else { it->certainty = (1.0 - DECAY_RATE)*it->certainty; }
					}

					if (containing.empty()) {
						if (max_cert < 0)
							max_point = last_point;
						Circle c;
						c.center = max_point;
						c.certainty = DECAY_RATE;
						circles.push_back(c);
					}
					else {
						double anti_cert = 1.0;
						for (vector<Circle>::iterator it = containing.begin(); it != containing.end(); ++it) {
							anti_cert *= 1.0 - (*it).certainty;
							if (it != containing.begin()) {
								list<Circle>::iterator shit, next_shit = circles.begin();
								while (next_shit != circles.end()) {
									shit = next_shit;
									++next_shit;
									if (*shit == *it) {
										circles.erase(shit);
									}
								}
							}
						}
						anti_cert *= 1.0 - DECAY_RATE;
						containing[0].certainty = 1.0 - anti_cert;
					}

					for (list<Circle>::iterator it = circles.begin(); it != circles.end(); ) {
						if (it->certainty < DELETE_THRESHOLD) it = circles.erase(it);
						else ++it;
					}
				}

				max_cert = 0.0;
				list<Circle>::iterator max_point_it = circles.begin();
				for (list<Circle>::iterator it = circles.begin(); it != circles.end(); ++it) {
					if (max_cert < it->certainty) {
						max_cert = it->certainty;
						max_point_it = it;
					}
				}
				last_point = max_point_it->center;
				return max_point_it->center;
			}
	};

	ByronsFilter instance;
}

