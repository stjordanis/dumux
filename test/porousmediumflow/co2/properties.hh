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
 * \ingroup CO2Tests
 * \brief The properties of a problem, where CO2 is injected in a reservoir.
 */

#ifndef DUMUX_HETEROGENEOUS_PROPERTIES_HH
#define DUMUX_HETEROGENEOUS_PROPERTIES_HH


#include <dune/alugrid/grid.hh>

#include <dumux/discretization/cctpfa.hh>
#include <dumux/discretization/box.hh>
#include <dumux/porousmediumflow/co2/model.hh>

#include <dumux/material/components/tabulatedcomponent.hh>
#include <dumux/material/components/h2o.hh>
#include <dumux/material/fluidsystems/brineco2.hh>


#include "spatialparams.hh"
#include "co2tables.hh"

// per default use isothermal model
#ifndef ISOTHERMAL
#define ISOTHERMAL 1
#endif

#include "problem.hh"

namespace Dumux::Properties {
// Create new type tags
namespace TTag {
struct Heterogeneous { using InheritsFrom = std::tuple<TwoPTwoCCO2>; };
struct HeterogeneousBox { using InheritsFrom = std::tuple<Heterogeneous, BoxModel>; };
struct HeterogeneousCCTpfa { using InheritsFrom = std::tuple<Heterogeneous, CCTpfaModel>; };
} // end namespace TTag

//Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::Heterogeneous> { using type = Dune::ALUGrid<2, 2, Dune::cube, Dune::nonconforming>; };

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::Heterogeneous> { using type = HeterogeneousProblem<TypeTag>; };

// Set the spatial parameters
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::Heterogeneous>
{
    using type = HeterogeneousSpatialParams<GetPropType<TypeTag, Properties::GridGeometry>,
                                            GetPropType<TypeTag, Properties::Scalar>>;
};

// Set fluid configuration
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::Heterogeneous>
{
    using type = FluidSystems::BrineCO2<GetPropType<TypeTag, Properties::Scalar>,
                                        HeterogeneousCO2Tables::CO2Tables,
                                        Components::TabulatedComponent<Components::H2O<GetPropType<TypeTag, Properties::Scalar>>>,
                                        FluidSystems::BrineCO2DefaultPolicy</*constantSalinity=*/true, /*simpleButFast=*/true>>;
};

// Use Moles
template<class TypeTag>
struct UseMoles<TypeTag, TTag::Heterogeneous> { static constexpr bool value = false; };

#if !ISOTHERMAL
// Create new type tags
namespace TTag {
struct HeterogeneousNI { using InheritsFrom = std::tuple<TwoPTwoCCO2NI>; };
struct HeterogeneousNIBox { using InheritsFrom = std::tuple<HeterogeneousNI, BoxModel>; };
struct HeterogeneousNICCTpfa { using InheritsFrom = std::tuple<HeterogeneousNI, CCTpfaModel>; };
} // end namespace TTag

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::HeterogeneousNI> { using type = Dune::ALUGrid<2, 2, Dune::cube, Dune::nonconforming>; };

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::HeterogeneousNI> { using type = HeterogeneousProblem<TypeTag>; };

// Set the spatial parameters
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::HeterogeneousNI>
{
    using type = HeterogeneousSpatialParams<GetPropType<TypeTag, Properties::GridGeometry>,
                                            GetPropType<TypeTag, Properties::Scalar>>;
};

// Set fluid configuration
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::HeterogeneousNI>
{
    using type = FluidSystems::BrineCO2<GetPropType<TypeTag, Properties::Scalar>,
                                        HeterogeneousCO2Tables::CO2Tables,
                                        Components::TabulatedComponent<Components::H2O<GetPropType<TypeTag, Properties::Scalar>>>,
                                        FluidSystems::BrineCO2DefaultPolicy</*constantSalinity=*/true, /*simpleButFast=*/true>>;
};

// Use Moles
template<class TypeTag>
struct UseMoles<TypeTag, TTag::HeterogeneousNI> { static constexpr bool value = false; };
#endif
} // end namespace Dumux::Properties

#endif
