#ifndef AI_HL_STP_EVALUATION_RRT_PLANNER_H
#define AI_HL_STP_EVALUATION_RRT_PLANNER_H

#include "ai/hl/stp/world.h"
#include "util/object_store.h"
#include "geom/param.h"
#include <vector>
#include <memory>
#include <glibmm/nodetree.h>

namespace AI {
    namespace Nav {
        namespace RRT{

            class Waypoints final : public ObjectStore::Element {
                public:
                    AI::Flags::MoveFlags added_flags;
                    Timestamp lastSentTime;
                    Point move_dest;
            };

            /**
             * Determines how far an endpoint in the path is from the goal location
             */
            double distance(Glib::NodeTree<Point> *node, Point goal);

            Point random_point(World world);

            Point choose_target(World world, Point goal, Player player, std::vector<Point> way_points);

            Glib::NodeTree<Point> *nearest(Glib::NodeTree<Point> *tree, Point target);


            /**
             * This function decides how to move toward the target the target is one of a random point,
             * a waypoint, or the goal location. A subclass may override this
             */
            Point extend(World world, Player player, Glib::NodeTree<Point> *start, Point target, AI::Flags::MoveFlags added_flags);

            /**
             * Generates a path for a player given the goal
             * optional parameter post_process sets whether to try and smooth out the final path
             */
            std::vector<Point> rrt_plan(World world, Player player, Point goal, std::vector<Point> way_points, bool post_process = true, AI::Flags::MoveFlags added_flags = AI::Flags::MoveFlags::NONE);

            inline constexpr Point empty_state() {
                return Point(-10000, -10000);
            };
        }
    }
}


#endif 
