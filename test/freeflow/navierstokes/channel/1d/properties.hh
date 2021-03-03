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
 * \ingroup NavierStokesTests
 * \brief The properties of the test for the 1-D Navier-Stokes model with an analytical solution.
 */
#ifndef DUMUX_DONEA_TEST_PROPERTIES_HH
#define DUMUX_DONEA_TEST_PROPERTIES_HH

#include <dune/common/fmatrix.hh>
#include <dune/grid/yaspgrid.hh>
#include <dumux/discretization/staggered/freeflow/properties.hh>

#include <dumux/freeflow/navierstokes/model.hh>
#include <dumux/material/components/constant.hh>
#include <dumux/material/fluidsystems/1pliquid.hh>

#include "problem.hh"
namespace Dumux::Properties {
// Create new type tags
namespace TTag {
struct NavierStokesAnalytic { using InheritsFrom = std::tuple<NavierStokes, StaggeredFreeFlowModel>; };
} // end namespace TTag

// the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::NavierStokesAnalytic>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = FluidSystems::OnePLiquid<Scalar, Components::Constant<1, Scalar> >;
};

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::NavierStokesAnalytic> { using type = Dune::YaspGrid<1>; };

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::NavierStokesAnalytic> { using type = Dumux::NavierStokesAnalyticProblem<TypeTag> ; };

template<class TypeTag>
struct EnableGridGeometryCache<TypeTag, TTag::NavierStokesAnalytic> { static constexpr bool value = true; };

template<class TypeTag>
struct EnableGridFluxVariablesCache<TypeTag, TTag::NavierStokesAnalytic> { static constexpr bool value = true; };
template<class TypeTag>
struct EnableGridVolumeVariablesCache<TypeTag, TTag::NavierStokesAnalytic> { static constexpr bool value = true; };

template<class TypeTag>
struct NormalizePressure<TypeTag, TTag::NavierStokesAnalytic> { static constexpr bool value = false; };
} // end namespace Dumux::Properties

#endif
