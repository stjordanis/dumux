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
 * \brief The properties for the (d-1)-dimensional facet domain in the single-phase
 *        facet coupling test involving three domains.
 */

#ifndef DUMUX_TEST_FACETCOUPLING_THREEDOMAIN_ONEP_FACET_PROPERTIES_HH
#define DUMUX_TEST_FACETCOUPLING_THREEDOMAIN_ONEP_FACET_PROPERTIES_HH

#include <dune/foamgrid/foamgrid.hh>

#include <dumux/material/components/constant.hh>
#include <dumux/material/fluidsystems/1pliquid.hh>

#include <dumux/multidomain/facet/box/properties.hh>
#include <dumux/multidomain/facet/cellcentered/tpfa/properties.hh>
#include <dumux/multidomain/facet/cellcentered/mpfa/properties.hh>
#include <dumux/porousmediumflow/1p/model.hh>

#include "spatialparams.hh"

#include "problem_facet.hh"

namespace Dumux::Properties {
// create the type tag nodes
// Create new type tags
namespace TTag {
struct OnePFacet { using InheritsFrom = std::tuple<OneP>; };
struct OnePFacetTpfa { using InheritsFrom = std::tuple<CCTpfaFacetCouplingModel, OnePFacet>; };
struct OnePFacetMpfa { using InheritsFrom = std::tuple<CCMpfaFacetCouplingModel, OnePFacet>; };
struct OnePFacetBox { using InheritsFrom = std::tuple<BoxFacetCouplingModel, OnePFacet>; };
} // end namespace TTag

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::OnePFacet> { using type = Dune::FoamGrid<2, 3>; };
// Set the problem type
template<class TypeTag>
struct Problem<TypeTag, TTag::OnePFacet> { using type = OnePFacetProblem<TypeTag>; };
// set the spatial params
template<class TypeTag>
struct SpatialParams<TypeTag, TTag::OnePFacet>
{
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = OnePSpatialParams<GridGeometry, Scalar>;
};

// the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::OnePFacet>
{
private:
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
public:
    using type = FluidSystems::OnePLiquid< Scalar, Components::Constant<1, Scalar> >;
};

} // end namespace Dumux::Properties

#endif
