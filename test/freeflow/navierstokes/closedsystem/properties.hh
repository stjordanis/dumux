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
 * \brief The properties for the test problem for the staggered (Navier-) Stokes model.
 */
#ifndef DUMUX_CLOSEDSYSTEM_TEST_PROPERTIES_HH
#define DUMUX_CLOSEDSYSTEM_TEST_PROPERTIES_HH

#include <dune/grid/yaspgrid.hh>
#include <dumux/discretization/staggered/freeflow/properties.hh>

#include <dumux/freeflow/navierstokes/model.hh>
#include <dumux/material/components/constant.hh>
#include <dumux/material/fluidsystems/1pliquid.hh>

// the missing problem file
//#include "problem.hh"

namespace Dumux::Properties {
// Create new type tags
namespace TTag {
struct ClosedSystemTest { using InheritsFrom = std::tuple<NavierStokes, StaggeredFreeFlowModel>; };
} // end namespace TTag

// the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::ClosedSystemTest>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = FluidSystems::OnePLiquid<Scalar, Components::Constant<1, Scalar> >;
};

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::ClosedSystemTest> { using type = Dune::YaspGrid<2>; };

// Set the problem property
/* template<class TypeTag>
struct Problem<TypeTag, TTag::ClosedSystemTest> { using type = Dumux::ClosedSystemTestProblem<TypeTag> ; }; */

template<class TypeTag>
struct EnableGridGeometryCache<TypeTag, TTag::ClosedSystemTest> { static constexpr bool value = true; };

template<class TypeTag>
struct EnableGridFluxVariablesCache<TypeTag, TTag::ClosedSystemTest> { static constexpr bool value = true; };
template<class TypeTag>
struct EnableGridVolumeVariablesCache<TypeTag, TTag::ClosedSystemTest> { static constexpr bool value = true; };
} // end namespace Dumux::Properties

#endif
