/*
 * passMainLoop.h
 *
 *  Created on: 2015-05-23
 *      Author: cheng
 */

#ifndef PASSMAINLOOP_H_
#define PASSMAINLOOP_H_

#include "PassInfo.h"
#include "geom/angle.h"
#include "geom/point.h"

#include <atomic>

namespace AI
{
namespace HL
{
namespace STP
{
namespace GradientApproach
{
void superLoop(PassInfo::worldSnapshot, std::atomic_bool* stop_token);

void testLoop(PassInfo::worldSnapshot);
bool comparePassQuality(PassInfo::passDataStruct a, PassInfo::passDataStruct b);

// std::vector<std::vector<PassInfo::passDataStruct> > passPointsLog;

std::vector<PassInfo::passDataStruct> newPositions(
    PassInfo::worldSnapshot snapshot, unsigned int quantity);

PassInfo::passDataStruct stepForward(
    PassInfo::worldSnapshot snapshot, PassInfo::passDataStruct);
std::vector<PassInfo::passDataStruct> stepForward(
    PassInfo::worldSnapshot snapshot, std::vector<PassInfo::passDataStruct>);

PassInfo::passDataStruct estimateParams(
    PassInfo::worldSnapshot snapshot, int i,
    std::vector<Point> mergedEnemyPositions, int j,
    std::vector<double> friendlyRadii);

std::vector<PassInfo::passDataStruct> merge(
    std::vector<PassInfo::passDataStruct> potential_passes);

// returns a vector of best pass positions to be sent to PassInfo, takes into
// account passes blocking each other
std::vector<PassInfo::passDataStruct> bestPassPositions(
    PassInfo::worldSnapshot snapshot,
    std::vector<PassInfo::passDataStruct> potential_passes,
    unsigned int numberPositions);

} /* namespace GradientApproach */
} /* namespace STP */
} /* namespace HL */
} /* namespace AI */

#endif /* PASSMAINLOOP_H_ */
