/*! \file SamplerGenMonkeyPatch.h
 * \author Chris Dellin <cdellin@gmail.com>
 * \copyright 2015 Carnegie Mellon University
 * \copyright License: BSD
 */

/* requires:
#include <ompl/base/StateSampler.h>
 */

namespace ompl_lemur
{

boost::mt19937 & SamplerGenMonkeyPatch(ompl::base::StateSamplerPtr sampler);

} // namespace ompl_lemur
