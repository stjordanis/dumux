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
 * \ingroup BoundaryTests
 * \brief The properties for the incompressible test
 */
#ifndef DUMUX_MULTIDOMAIN_ONEP_TWOP_TEST_PROPERTIES_HH
#define DUMUX_MULTIDOMAIN_ONEP_TWOP_TEST_PROPERTIES_HH

#include <config.h>

#include <dumux/io/grid/gridmanager.hh>

#include <dumux/porousmediumflow/1p/model.hh>
#include <dumux/porousmediumflow/2p/model.hh>
#include <dumux/material/components/simpleh2o.hh>
#include <dumux/material/components/ch4.hh>
#include <dumux/material/fluidsystems/1pliquid.hh>
#include <dumux/material/fluidsystems/1pgas.hh>
#include <dumux/material/fluidsystems/2pimmiscible.hh>
#include <dumux/discretization/cctpfa.hh>

#include <dumux/multidomain/traits.hh>
#include <dumux/multidomain/boundary/darcydarcy/couplingmanager.hh>

#include "problem.hh"

namespace Dumux::Properties {

// Create new type tags
namespace TTag {
struct OnePSub { using InheritsFrom = std::tuple<CCTpfaModel>; };
// differentiate between the two subproblems
struct OnePSub0 { using InheritsFrom = std::tuple<OneP, OnePSub>; };
struct OnePSub1 { using InheritsFrom = std::tuple<TwoP, OnePSub>; };
} // end namespace TTag

// the coupling manager
template<class TypeTag>
struct CouplingManager<TypeTag, TTag::OnePSub>
{ using type = DarcyDarcyBoundaryCouplingManager<MultiDomainTraits<Properties::TTag::OnePSub0, Properties::TTag::OnePSub1>>; };

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::OnePSub>
{
    using HostGrid = Dune::YaspGrid<2, Dune::EquidistantOffsetCoordinates<double, 2>>;
    using type = Dune::SubGrid<HostGrid::dimension, HostGrid>;
};

// set the spatial params
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::OnePSub>
{
    using type = TestSpatialParams<GetPropType<TypeTag, Properties::GridGeometry>,
                                   GetPropType<TypeTag, Properties::Scalar>>;
};

// differentiate between the two fluid systems
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::OnePSub0>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = FluidSystems::OnePLiquid<Scalar, Components::SimpleH2O<Scalar> >;
};

template<class TypeTag>
struct FluidSystem<TypeTag, TTag::OnePSub1>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = FluidSystems::TwoPImmiscible<Scalar, FluidSystems::OnePLiquid<Scalar, Components::SimpleH2O<Scalar>>,
                                                      FluidSystems::OnePGas<Scalar, Components::CH4<Scalar>>>;
};

// differentiate between the two subproblems
template<class TypeTag>
struct Problem<TypeTag, TTag::OnePSub0> { using type = OnePTestProblem<TypeTag, 0>; };
template<class TypeTag>
struct Problem<TypeTag, TTag::OnePSub1> { using type = OnePTestProblem<TypeTag, 1>; };

} // end namespace Dumux::Properties

#endif
