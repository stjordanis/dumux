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
 * \brief The properties for the one-phase soil problem.
 */
#ifndef DUMUX_TISSUE_PROPERTIES_HH
#define DUMUX_TISSUE_PROPERTIES_HH



#include <dune/grid/yaspgrid.hh>
#include <dune/grid/uggrid.hh>
#include <dumux/common/properties.hh>
#include <dumux/discretization/cctpfa.hh>
#include <dumux/porousmediumflow/richardsnc/model.hh>
#include <dumux/material/components/simpleh2o.hh>
#include <dumux/material/components/constant.hh>
#include <dumux/material/fluidsystems/liquidphase2c.hh>
#include "problem_soil.hh"
namespace Dumux::Properties {

// Create new type tags
namespace TTag {
struct Soil { using InheritsFrom = std::tuple<RichardsNC, CCTpfaModel>; };
} // end namespace TTag

// Set the grid type
#if HAVE_UG
template<class TypeTag>
struct Grid<TypeTag, TTag::Soil> { using type = Dune::UGGrid<3>; };
#else
template<class TypeTag>
struct Grid<TypeTag, TTag::Soil> { using type = Dune::YaspGrid<3, Dune::EquidistantOffsetCoordinates<double, 3>>; };
#endif

template<class TypeTag>
struct EnableGridGeometryCache<TypeTag, TTag::Soil> { static constexpr bool value = true; };
template<class TypeTag>
struct EnableGridVolumeVariablesCache<TypeTag, TTag::Soil> { static constexpr bool value = true; };
template<class TypeTag>
struct EnableGridFluxVariablesCache<TypeTag, TTag::Soil> { static constexpr bool value = true; };
template<class TypeTag>
struct SolutionDependentAdvection<TypeTag, TTag::Soil> { static constexpr bool value = false; };
template<class TypeTag>
struct SolutionDependentMolecularDiffusion<TypeTag, TTag::Soil> { static constexpr bool value = false; };
template<class TypeTag>
struct SolutionDependentHeatConduction<TypeTag, TTag::Soil> { static constexpr bool value = false; };

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::Soil> { using type = SoilProblem<TypeTag>; };

// Set the spatial parameters
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::Soil>
{
    using type = SoilSpatialParams<GetPropType<TypeTag, Properties::GridGeometry>,
                                   GetPropType<TypeTag, Properties::Scalar>>;
};

// Set the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::Soil>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = FluidSystems::LiquidPhaseTwoC<Scalar, Components::SimpleH2O<Scalar>,
                                                       Components::Constant<1, Scalar>>;
};

template<class TypeTag>
struct UseMoles<TypeTag, TTag::Soil> { static constexpr bool value = true; };

} // end namespace Dumux::Properties

#endif
