#include "ai/navigator/spline_planner.h"
#include <memory>
#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::Nav;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;

const unsigned NUM_PATHPOINTS = 7;
    
namespace {
    inline double splineWt(int ptNum, double t){
        switch(ptNum){
            case 0:
                return (1-t)*(1-t)*(1-t);
            case 1:
                return 3*t*(1-t)*(1-t);
            case 2:
                return 3*t*t*(1-t);
            case 3:
                return t*t*t;
            default:
                return 0.0;
        }
    }
    std::vector<Point> getBezierPath(std::vector<Point> ctrlPts){
        std::vector<Point> path;
        for(unsigned i=1;i<=NUM_PATHPOINTS;i++){
                double t = (double)i/(NUM_PATHPOINTS);
                path.push_back(splineWt(0,t)*ctrlPts.at(0) + splineWt(1,t)*ctrlPts.at(1) +splineWt(2,t)*ctrlPts.at(2) + splineWt(3,t)*ctrlPts.at(3));
        }
        return path;
    }
}



std::vector<Point> SplinePlanner::plan(Player player, Point goal, MoveFlags added_flags)
{
    Point initial = player.position();// + player.velocity()*player.velocity().len()/(2.0*3.0);
    Point mid = (initial + goal)/2;
    double interDist = 0.1;
    double maxDist = 1.5;
    std::vector<Point> ctrlPts = {player.position(), initial, mid, goal};

    std::vector<Point> path = getBezierPath(ctrlPts);
    if(valid_path(path,world,player,added_flags)){
        return ctrlPts;
    }

    Point normal = (goal - initial).rotate(Angle::quarter()).norm();
    for(double r=0.0;r<=maxDist;r+=interDist){
        ctrlPts.at(2) = mid + normal*r;
        path = getBezierPath(ctrlPts);
        if(valid_path(path,world,player,added_flags)){
            return path;
        }
        ctrlPts.at(2) = mid - normal*r;
        path = getBezierPath(ctrlPts);
        if(valid_path(path,world,player,added_flags)){
            return path;
        }
    }
    path.clear(); // fail- return empty vector
    return path;
}


SplinePlanner::SplinePlanner(World world) : Plan(world)
{
}
