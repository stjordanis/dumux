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
 * \brief A Darcy test problem using Maxwell-Stefan diffusion.
 */

#ifndef STOKESDARCY_1P3C_PROPERTIES_HH
#define STOKESDARCY_1P3C_PROPERTIES_HH

#include <dune/grid/yaspgrid.hh>

#include <dumux/discretization/cctpfa.hh>
#include <dumux/discretization/staggered/freeflow/properties.hh>

#include <dumux/porousmediumflow/1pnc/model.hh>
#include <dumux/freeflow/compositional/navierstokesncmodel.hh>

#include "h2n2co2fluidsystem.hh"
#include "problem_darcy.hh"
#include "problem_stokes.hh"

namespace Dumux{
////////////////////////////////////////////////////
/////Darcyproblem///////////////////////////////////
////////////////////////////////////////////////////
namespace Properties {

// Create new type tags
namespace TTag {
struct DarcyOnePThreeC { using InheritsFrom = std::tuple<OnePNC, CCTpfaModel>; };
} // end namespace TTag

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::DarcyOnePThreeC> { using type = Dumux::DarcySubProblem<TypeTag>; };

template<class TypeTag>
struct FluidSystem<TypeTag, TTag::DarcyOnePThreeC> { using type = FluidSystems::H2N2CO2FluidSystem<GetPropType<TypeTag, Properties::Scalar>>; };

// Use moles
template<class TypeTag>
struct UseMoles<TypeTag, TTag::DarcyOnePThreeC> { static constexpr bool value = true; };

// Do not replace one equation with a total mass balance
template<class TypeTag>
struct ReplaceCompEqIdx<TypeTag, TTag::DarcyOnePThreeC> { static constexpr int value = 3; };

//! Use a model with constant tortuosity for the effective diffusivity
template<class TypeTag>
struct EffectiveDiffusivityModel<TypeTag, TTag::DarcyOnePThreeC>
{ using type = DiffusivityConstantTortuosity<GetPropType<TypeTag, Properties::Scalar>>; };

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::DarcyOnePThreeC> { using type = Dune::YaspGrid<2>; };

//Set the diffusion type
template<class TypeTag>
struct MolecularDiffusionType<TypeTag, TTag::DarcyOnePThreeC> { using type = MaxwellStefansLaw<TypeTag>; };

// Set the spatial paramaters type
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::DarcyOnePThreeC>
{
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = OnePSpatialParams<GridGeometry, Scalar>;
};

} // end namespace Properties

////////////////////////////////////////////////////
/////Stokesproblem//////////////////////////////////
////////////////////////////////////////////////////

namespace Properties {
// Create new type tags
namespace TTag {
struct StokesOnePThreeC { using InheritsFrom = std::tuple<NavierStokesNC, StaggeredFreeFlowModel>; };
} // end namespace TTag

// Set the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::StokesOnePThreeC> { using type = FluidSystems::H2N2CO2FluidSystem<GetPropType<TypeTag, Properties::Scalar>>; };

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::StokesOnePThreeC> { using type = Dune::YaspGrid<2, Dune::EquidistantOffsetCoordinates<GetPropType<TypeTag, Properties::Scalar>, 2> >; };

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::StokesOnePThreeC> { using type = Dumux::StokesSubProblem<TypeTag> ; };

template<class TypeTag>
struct EnableGridGeometryCache<TypeTag, TTag::StokesOnePThreeC> { static constexpr bool value = true; };
template<class TypeTag>
struct EnableGridFluxVariablesCache<TypeTag, TTag::StokesOnePThreeC> { static constexpr bool value = true; };
template<class TypeTag>
struct EnableGridVolumeVariablesCache<TypeTag, TTag::StokesOnePThreeC> { static constexpr bool value = true; };

// Use moles
template<class TypeTag>
struct UseMoles<TypeTag, TTag::StokesOnePThreeC> { static constexpr bool value = true; };

// Set the grid type
template<class TypeTag>
struct MolecularDiffusionType<TypeTag, TTag::StokesOnePThreeC> { using type = MaxwellStefansLaw<TypeTag>; };

// Do not replace one equation with a total mass balance
template<class TypeTag>
struct ReplaceCompEqIdx<TypeTag, TTag::StokesOnePThreeC> { static constexpr int value = 3; };
} // end namespace Properties
} // end namespace Dumux
#endif
