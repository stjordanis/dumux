// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \ingroup OnePTests
 * \brief The properties for the convergence test with analytic solution
 */
#ifndef DUMUX_CONVERGENCE_SPHERE_TEST_ONEP_PROPERTIES_HH
#define DUMUX_CONVERGENCE_SPHERE_TEST_ONEP_PROPERTIES_HH

#include "../analyticsolution/properties.hh"

#include "problem.hh"
#include "spatialparams.hh"

namespace Dumux::Properties {

// Create new type tags
namespace TTag {
struct OnePConvergenceSphere { using InheritsFrom = std::tuple<TYPETAG>; };
} // end namespace TTag

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::OnePConvergenceSphere>
{ using type = ConvergenceSphereProblem<TypeTag>; };

// Set the problem property
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::OnePConvergenceSphere>
{
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = ConvergenceSphereSpatialParams<GridGeometry, Scalar>;
};

} // end namespace Dumux::Properties

#endif
