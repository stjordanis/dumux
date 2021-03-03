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
 * \ingroup TwoPTests
 * \brief The properties for the 2p cornerpoint test.
 */

#ifndef DUMUX_TWOP_CORNERPOINT_TEST_PROBLEM_PROPERTIES_HH
#define DUMUX_TWOP_CORNERPOINT_TEST_PROBLEM_PROPERTIES_HH

#if HAVE_OPM_GRID
#include <opm/grid/CpGrid.hpp>

#include <dumux/discretization/cctpfa.hh>

#include <dumux/material/components/trichloroethene.hh>
#include <dumux/material/components/simpleh2o.hh>
#include <dumux/material/fluidsystems/1pliquid.hh>
#include <dumux/material/fluidsystems/2pimmiscible.hh>


#include "problem.hh"
#include "spatialparams.hh"

namespace Dumux::Properties {
// Create new type tags
namespace TTag {
struct TwoPCornerPoint { using InheritsFrom = std::tuple<CCTpfaModel, TwoP>; };
} // end namespace TTag

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::TwoPCornerPoint> { using type = Dune::CpGrid; };

// Set the problem type
template<class TypeTag>
struct Problem<TypeTag, TTag::TwoPCornerPoint> { using type = TwoPCornerPointTestProblem<TypeTag>; };

// the local residual containing the analytic derivative methods
template<class TypeTag>
struct LocalResidual<TypeTag, TTag::TwoPCornerPoint> { using type = TwoPIncompressibleLocalResidual<TypeTag>; };

// Set the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::TwoPCornerPoint>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using WettingPhase = FluidSystems::OnePLiquid<Scalar, Components::SimpleH2O<Scalar> >;
    using NonwettingPhase = FluidSystems::OnePLiquid<Scalar, Components::Trichloroethene<Scalar> >;
    using type = FluidSystems::TwoPImmiscible<Scalar, WettingPhase, NonwettingPhase>;
};

// Set the spatial parameters
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::TwoPCornerPoint>
{
private:
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
public:
    using type = TwoPCornerPointTestSpatialParams<GridGeometry, Scalar>;
};

// Enable caching
template<class TypeTag>
struct EnableGridVolumeVariablesCache<TypeTag, TTag::TwoPCornerPoint> { static constexpr bool value = false; };
template<class TypeTag>
struct EnableGridFluxVariablesCache<TypeTag, TTag::TwoPCornerPoint> { static constexpr bool value = false; };
template<class TypeTag>
struct EnableGridGeometryCache<TypeTag, TTag::TwoPCornerPoint> { static constexpr bool value = false; };
} // end namespace Properties

#else
#warning "The opm-grid module is needed to use this class!"
#endif

#endif
