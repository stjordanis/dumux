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
 * \ingroup FacetTests
 * \brief The properties for the (d-2)-dimensional edge domain in the single-phase
 *        facet coupling test involving three domains.
 */

#ifndef DUMUX_TEST_FACETCOUPLING_THREEDOMAIN_ONEP_EDGE_PROPERTIES_HH
#define DUMUX_TEST_FACETCOUPLING_THREEDOMAIN_ONEP_EDGE_PROPERTIES_HH

#include <dune/foamgrid/foamgrid.hh>

#include <dumux/material/components/constant.hh>
#include <dumux/material/fluidsystems/1pliquid.hh>

#include <dumux/discretization/box.hh>
#include <dumux/discretization/cctpfa.hh>
#include <dumux/discretization/ccmpfa.hh>

#include <dumux/porousmediumflow/1p/model.hh>

#include "spatialparams.hh"

#include "problem_edge.hh"

namespace Dumux::Properties {
// create the type tag nodes
// Create new type tags
namespace TTag {
struct OnePEdge { using InheritsFrom = std::tuple<OneP>; };
struct OnePEdgeTpfa { using InheritsFrom = std::tuple<OnePEdge, CCTpfaModel>; };
struct OnePEdgeMpfa { using InheritsFrom = std::tuple<OnePEdge, CCTpfaModel>; };
struct OnePEdgeBox { using InheritsFrom = std::tuple<OnePEdge, BoxModel>; };
} // end namespace TTag

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::OnePEdge> { using type = Dune::FoamGrid<1, 3>; };
// Set the problem type
template<class TypeTag>
struct Problem<TypeTag, TTag::OnePEdge> { using type = OnePEdgeProblem<TypeTag>; };
// set the spatial params
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::OnePEdge>
{
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = OnePSpatialParams<GridGeometry, Scalar>;
};

// the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::OnePEdge>
{
private:
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
public:
    using type = FluidSystems::OnePLiquid< Scalar, Components::Constant<1, Scalar> >;
};

} // end namespace Dumux::Properties

#endif
