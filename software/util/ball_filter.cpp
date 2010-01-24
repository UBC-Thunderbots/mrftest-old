#include <complex>
#include <list>
#include <utility>
#include <vector>

using namespace std;

typedef complex<double> pt;

struct circle {
    pt center;
    double certainty;
	
	bool operator==(circle other) {
		return (center == other.center);
	}
};

class ball_filter {
    private:
        static const double RADIUS = 0.6;
        #warning stupid douchebags stop using magic timing constants if it
        #warning is in relation to time give in relation to FPS
        static const double DECAY_RATE = 0.2063; // half-life = 3 frames 
        static const double DELETE_THRESHOLD = 0.02; // stores < 50 circles
        static list<circle> circles;
    
    public:
        static void init_filter() {
            circles.clear();
            circle c;
            c.center = pt(0, 0);
            c.certainty = DELETE_THRESHOLD;
            circles.push_back(c);
        }
        
        static pt filter(const vector<pair<pt, double> >& obs) {
          pt max_point;
          double max_cert = 0.0;
          
          if (obs.empty()) {
            for (list<circle>::iterator it = circles.begin(); it != circles.end(); it++) {
                it->certainty = (1.0 - DECAY_RATE)*it->certainty;
            }
          }
          else {
            for (int i = 0; i < int(obs.size()); i++) {
                if (max_cert < obs[i].second) {
                    max_point = obs[i].first;
                    max_cert = obs[i].second;
                }
            }
            
            vector<circle> containing;
            for (list<circle>::iterator it = circles.begin(); it != circles.end(); it++) {
                if (abs(max_point - it->center) < RADIUS) {
                    containing.push_back(*it);
                    it->center = max_point;
                }
                else { it->certainty = (1.0 - DECAY_RATE)*it->certainty; }
            }
            
            if (containing.empty()) {

                circle c;
                c.center = max_point;
                c.certainty = DECAY_RATE;
                circles.push_back(c);
            }
            else {
                double anti_cert = 1.0;
                for (vector<circle>::iterator it = containing.begin(); it != containing.end(); ++it) {
                    anti_cert *= 1.0 - (*it).certainty;
                    if (it != containing.begin()) {
						for (list<circle>::iterator shit = circles.begin(); shit != circles.end(); ++shit) {
							if( (*shit) == (*it) )
		                        circles.erase(shit);
						}	

					}
                }
                anti_cert *= 1.0 - DECAY_RATE;
                containing[0].certainty = 1.0 - anti_cert;
            }
            
            for (list<circle>::iterator it = circles.begin(); it != circles.end(); ) {
                if (it->certainty < DELETE_THRESHOLD) it = circles.erase(it);
                else it++;
            }
          }
            
            max_cert = 0.0;
            list<circle>::iterator max_point_it = circles.begin();
            for (list<circle>::iterator it = circles.begin(); it != circles.end(); it++) {
                if (max_cert < it->certainty) {
                    max_cert = it->certainty;
                    max_point_it = it;
                }
            }
            return max_point_it->center;
        }
};
