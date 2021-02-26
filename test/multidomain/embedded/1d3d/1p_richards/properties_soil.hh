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
 * \brief The properties for the one-phase soil problem
 */
#ifndef DUMUX_TISSUE_PROPERTIES_HH
#define DUMUX_TISSUE_PROPERTIES_HH

#include <dune/grid/yaspgrid.hh>
#include <dumux/common/properties.hh>
#include <dumux/discretization/cctpfa.hh>
#include <dumux/discretization/box.hh>
#include <dumux/porousmediumflow/richards/model.hh>

#include "problem_soil.hh"

namespace Dumux::Properties {

// Create new type tags
namespace TTag {
struct Soil { using InheritsFrom = std::tuple<Richards>; };
struct SoilCC { using InheritsFrom = std::tuple<Soil, CCTpfaModel>; };
struct SoilBox { using InheritsFrom = std::tuple<Soil, BoxModel>; };
} // end namespace TTag

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::Soil> { using type = Dune::YaspGrid<3, Dune::EquidistantOffsetCoordinates<GetPropType<TypeTag, Properties::Scalar>, 3> >; };

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
} // end namespace Dumux::Properties

#endif
