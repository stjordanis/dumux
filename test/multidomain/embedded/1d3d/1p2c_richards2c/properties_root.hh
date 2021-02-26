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
 * \ingroup EmbeddedTests
 * \brief The properties of a test problem for the one-phase root model.
 */
#ifndef DUMUX_ROOT_PROPERTIES_HH
#define DUMUX_ROOT_PROPERTIES_HH

#include <dune/foamgrid/foamgrid.hh>

#include <dumux/common/properties.hh>
#include <dumux/discretization/cctpfa.hh>
#include <dumux/porousmediumflow/1pnc/model.hh>
#include <dumux/material/components/simpleh2o.hh>
#include <dumux/material/components/constant.hh>
#include <dumux/material/fluidsystems/liquidphase2c.hh>

#include "problem_root.hh"

namespace Dumux::Properties {

// Create new type tags
namespace TTag {
struct Root { using InheritsFrom = std::tuple<OnePNC, CCTpfaModel>; };
} // end namespace TTag

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::Root> { using type = Dune::FoamGrid<1, 3>; };

template<class TypeTag>
struct EnableGridGeometryCache<TypeTag, TTag::Root> { static constexpr bool value = true; };
template<class TypeTag>
struct EnableGridVolumeVariablesCache<TypeTag, TTag::Root> { static constexpr bool value = true; };
template<class TypeTag>
struct EnableGridFluxVariablesCache<TypeTag, TTag::Root> { static constexpr bool value = true; };
template<class TypeTag>
struct SolutionDependentAdvection<TypeTag, TTag::Root> { static constexpr bool value = false; };
template<class TypeTag>
struct SolutionDependentMolecularDiffusion<TypeTag, TTag::Root> { static constexpr bool value = false; };
template<class TypeTag>
struct SolutionDependentHeatConduction<TypeTag, TTag::Root> { static constexpr bool value = false; };

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::Root> { using type = RootProblem<TypeTag>; };

// Set the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::Root>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = FluidSystems::LiquidPhaseTwoC<Scalar, Components::SimpleH2O<Scalar>,
                                                       Components::Constant<1, Scalar>>;
};

// Set the spatial parameters
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::Root>
{
    using type = RootSpatialParams<GetPropType<TypeTag, Properties::GridGeometry>,
                                   GetPropType<TypeTag, Properties::Scalar>>;
};

template<class TypeTag>
struct UseMoles<TypeTag, TTag::Root> { static constexpr bool value = true; };

} // end namespace Dumux::Properties

#endif
