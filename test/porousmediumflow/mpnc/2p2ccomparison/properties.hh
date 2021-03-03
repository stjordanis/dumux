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
 * \ingroup MPNCTests
 * \brief The properties of the problem where air is injected in a unsaturated porous medium.
 */
#ifndef DUMUX_MPNC_TWOPTWOC_COMPARISON_OBSTACLEPROPERTIES_HH
#define DUMUX_MPNC_TWOPTWOC_COMPARISON_OBSTACLEPROPERTIES_HH

#include <dune/grid/yaspgrid.hh>
#include <dumux/discretization/box.hh>
#include <dumux/discretization/cctpfa.hh>

#include <dumux/porousmediumflow/mpnc/model.hh>
#include <dumux/material/fluidsystems/h2on2.hh>
#include <dumux/material/fluidstates/compositional.hh>


#include "spatialparams.hh"

#include "problem.hh"

namespace Dumux::Properties {
// Create new type tags
namespace TTag {
struct MPNCComparison { using InheritsFrom = std::tuple<MPNC>; };
struct MPNCComparisonBox { using InheritsFrom = std::tuple<MPNCComparison, BoxModel>; };
struct MPNCComparisonCC { using InheritsFrom = std::tuple<MPNCComparison, CCTpfaModel>; };
} // end namespace TTag

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::MPNCComparison> { using type = Dune::YaspGrid<2>; };

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::MPNCComparison> { using type = MPNCComparisonProblem<TypeTag>; };

// Set the spatial parameters
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::MPNCComparison>
{
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = MPNCComparisonSpatialParams<GridGeometry, Scalar>;
};

// Set fluid configuration
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::MPNCComparison>
{
    using type = FluidSystems::H2ON2<GetPropType<TypeTag, Properties::Scalar>,
                                     FluidSystems::H2ON2DefaultPolicy</*fastButSimplifiedRelations=*/true>>;
};

// decide which type to use for floating values (double / quad)
template<class TypeTag>
struct Scalar<TypeTag, TTag::MPNCComparison> { using type = double; };
template<class TypeTag>
struct UseMoles<TypeTag, TTag::MPNCComparison> { static constexpr bool value = true; };
template<class TypeTag>
struct IOFields<TypeTag, TTag::MPNCComparison> { using type = TwoPTwoCMPNCIOFields; };
} // end namespace Dumux::Properties

#endif
