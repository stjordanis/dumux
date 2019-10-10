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
 * \ingroup FacetCoupling
 * \brief Properties (and default properties) for all models using the box
 *        scheme together with coupling across the grid element facets
 * \note If n is the dimension of the lowest grid to be considered in the hierarchy,
 *       all problem type tags for the grids with the dimension m > n must inherit
 *       from these or other facet coupling properties (e.g. CCTpfaFacetCouplingModel).
 */

#ifndef DUMUX_FACETCOUPLING_BOX_PROPERTIES_HH
#define DUMUX_FACETCOUPLING_BOX_PROPERTIES_HH

#include <dumux/common/properties.hh>
#include <dumux/discretization/box.hh>

#include <dumux/multidomain/facet/box/darcyslaw.hh>
#include <dumux/multidomain/facet/box/elementboundarytypes.hh>
#include <dumux/multidomain/facet/box/fvgridgeometry.hh>
#include <dumux/multidomain/facet/box/localresidual.hh>
#include <dumux/multidomain/facet/box/upwindscheme.hh>

#include <dumux/porousmediumflow/fluxvariables.hh>

namespace Dumux {

namespace Properties {

//! Type tag for the box scheme with coupling to
//! another sub-domain living on the grid facets.
// Create new type tags
namespace TTag {
struct BoxFacetCouplingModel { using InheritsFrom = std::tuple<BoxModel>; };
} // end namespace TTag

//! Use the box local residual for models with facet coupling
template<class TypeTag>
struct BaseLocalResidual<TypeTag, TTag::BoxFacetCouplingModel> { using type = BoxFacetCouplingLocalResidual<TypeTag>; };

//! Use the box facet coupling-specific Darcy's law
template<class TypeTag>
struct AdvectionType<TypeTag, TTag::BoxFacetCouplingModel>
{
    using type = BoxFacetCouplingDarcysLaw< GetPropType<TypeTag, Properties::Scalar>,
                                            GetPropType<TypeTag, Properties::GridGeometry> >;
};

//! Per default, use the porous medium flow flux variables with the modified upwind scheme
template<class TypeTag>
struct FluxVariables<TypeTag, TTag::BoxFacetCouplingModel>
{
    using type = PorousMediumFluxVariables<TypeTag,
                                           BoxFacetCouplingUpwindScheme<GetPropType<TypeTag, Properties::GridGeometry>>>;
};

//! Per default, use the porous medium flow flux variables with the modified upwind scheme
template<class TypeTag>
struct ElementBoundaryTypes<TypeTag, TTag::BoxFacetCouplingModel>
{ using type = BoxFacetCouplingElementBoundaryTypes<GetPropType<TypeTag, Properties::BoundaryTypes>>; };

// Dumux 3.1 changes the property `FVGridGeometry` to `GridGeometry`.
// For ensuring backward compatibility on the user side, it is necessary to
// stick to the old name for the specializations, see the discussion in MR 1647.
// Use diagnostic pragmas to prevent the emission of a warning message.
// TODO after 3.1: Rename to GridGeometry, remove the pragmas and this comment.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//! Set the default for the grid finite volume geometry
template<class TypeTag>
struct FVGridGeometry<TypeTag, TTag::BoxFacetCouplingModel>
{
private:
    static constexpr bool enableCache = getPropValue<TypeTag, Properties::EnableGridGeometryCache>();
    using GridView = GetPropType<TypeTag, Properties::GridView>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
public:
    using type = BoxFacetCouplingFVGridGeometry<Scalar, GridView, enableCache>;
};
#pragma GCC diagnostic pop

} // namespace Properties
} // namespace Dumux

#endif
