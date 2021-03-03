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
 * \ingroup TwoPTests
 * \brief Non-isothermal gas injection problem where a gas (e.g. air) is injected into a fully
 *        water saturated medium.
 *
 * During buoyancy driven upward migration the gas passes a high temperature area.
 */

#ifndef DUMUX_INJECTION_PROBLEM_2PNI_HH
#define DUMUX_INJECTION_PROBLEM_2PNI_HH

#if HAVE_DUNE_ALUGRID
#include <dune/alugrid/grid.hh>
#endif
#if HAVE_UG
#include <dune/grid/uggrid.hh>
#endif
#include <dune/grid/yaspgrid.hh>

#include <dumux/common/properties.hh>

#include <dumux/porousmediumflow/2p/model.hh>

#include <dumux/discretization/box.hh>
#include <dumux/discretization/cctpfa.hh>

#include <dumux/material/fluidsystems/h2on2.hh>
#include <dumux/material/components/n2.hh>

// use the spatial parameters as the injection problem of the 2p2c test program
#include "problem.hh"
#include <test/porousmediumflow/2p2c/injection/spatialparams.hh>

#ifndef GRIDTYPE // default to yasp grid if not provided by CMake
#define GRIDTYPE Dune::YaspGrid<2>
#endif

namespace Dumux {

//! Forward declaration of the problem class
template <class TypeTag> class InjectionProblem2PNI;

namespace Properties {
// Create new type tags
namespace TTag {
struct Injection2PNITypeTag { using InheritsFrom = std::tuple<TwoPNI>; };
struct InjectionBox2PNITypeTag { using InheritsFrom = std::tuple<Injection2PNITypeTag, BoxModel>; };
struct InjectionCC2PNITypeTag { using InheritsFrom = std::tuple<Injection2PNITypeTag, CCTpfaModel>; };
} // end namespace TTag

// Obtain grid type from COMPILE_DEFINITIONS
template<class TypeTag>
struct Grid<TypeTag, TTag::Injection2PNITypeTag> { using type = GRIDTYPE; };

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::Injection2PNITypeTag> { using type = InjectionProblem2PNI<TypeTag>; };

// Use the same fluid system as the 2p2c injection problem
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::Injection2PNITypeTag> { using type = FluidSystems::H2ON2<GetPropType<TypeTag, Properties::Scalar>, FluidSystems::H2ON2DefaultPolicy</*fastButSimplifiedRelations=*/true>>; };

// Set the spatial parameters
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::Injection2PNITypeTag>
{
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = InjectionSpatialParams<GridGeometry, Scalar>;
};

} // namespace Properties
}
#endif
