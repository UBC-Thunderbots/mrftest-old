#ifndef AI_HL_STP_EVALUATION_INDIRECTCHIP_H_
#define AI_HL_STP_EVALUATION_INDIRECTCHIP_H_

#include <cstddef>
#include <vector>
#include "ai/hl/stp/world.h"
#include "geom/point.h"
#include "geom/shapes.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Evaluation
{
typedef Point Vector2;
template <size_t N>
using Poly     = std::array<Vector2, N>;
using Triangle = Poly<3>;

/* Returns the point to which the player taking the friendly
 * indirect kick should chip to, to chip over the first blocker (and aim at the
 * net)
 */
std::pair<Point, Angle> indirect_chip_target(World world, Player player);

/* Returns the point at which the player should shoot to deflect the ball of an
 * enemy
 * to the outside of the field to get another kick/corner
 */
Point deflect_off_enemy_target(World world);

/* Returns the target point to chip and chase at. The target the chipper should
 * shoot at
 * and the chaser should be prepared to meet the ball
 * The target is where the ball will land (according to chipping calibration)
 */
std::pair<Point, bool> indirect_chipandchase_target(World world);

/* Creates a vector of triangles, picking all permutations of points from the
 * list of all non-goalie
 * enemy players, and the 4 points returned by get_chip_area_target_corners
 */
std::vector<Triangle> get_all_triangles(
    World world, std::vector<Point> enemy_players);

/* Given a vector of triangles, returns the largest triangles with area greater
 * than min_area, and all edge lengths
 * greater than min_edge_len. The bool in the pair will be true if such a
 * triangle is found, and false otherwise.
 */
std::pair<Triangle, bool> get_largest_triangle(
    std::vector<Triangle> allTriangles, double min_area = 0,
    double min_edge_len = 0, double min_edge_angle = 0);

/* Given a vector of Triangles, returns a new vector only containing triangles
 * that do not contain enemy robots
 */
std::vector<Triangle> filter_open_triangles(
    std::vector<Triangle> triangles, std::vector<Point> enemy_players);

/* Returns four Points representing a rectangle within which we want to chip and
 * chase.
 * All points are 'inset' distance away from each edge of the field, to allow a
 * buffer for catching
 * and preventing the ball from leaving the field.
 * This rectangle is from the ball's position to the enemy's end of the field.
 */
std::vector<Point> get_chip_target_area_corners(World world, double inset);

/* Given a list of Triangles, removes all Triangles whose centers do not fall
 * within the rectangle
 * returned by get_chip_target_area
 */
std::vector<Triangle> remove_outofbounds_triangles(
    World world, std::vector<Triangle> triangles);

/* Returns the center Point and area of the given triangle
 */
std::pair<Point, double> get_triangle_center_and_area(Triangle triangle);
}
}
}
}

#endif
