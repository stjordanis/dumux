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
 * \ingroup PorenetworkFlow
 * \file
 *
 * \brief Defines common properties required for all pore-network models.
 */
#ifndef DUMUX_PNM_COMMON_PROPERTIES_HH
#define DUMUX_PNM_COMMON_PROPERTIES_HH

#include <dumux/common/properties/model.hh>
#include <dumux/discretization/box.hh>
#include <dumux/discretization/porenetwork/gridgeometry.hh>
#include <dumux/flux/porenetwork/fourierslaw.hh>
#include <dumux/porousmediumflow/fluxvariables.hh>

#include <dumux/porenetworkflow/common/labels.hh>
#include <dumux/porenetworkflow/common/velocityoutput.hh>
#include <dumux/porenetworkflow/common/utilities.hh>
#include <dumux/porenetworkflow/common/throatproperties.hh>

namespace Dumux {
namespace Properties {

//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////

//! The type tag for the pore-network problem
// Create new type tags
namespace TTag {
// TODO Do we want to inherit from box? Or is this a completely new discretization?
struct PoreNetworkModel { using InheritsFrom = std::tuple<ModelProperties, BoxModel>; };
} // end namespace TTag

//////////////////////////////////////////////////////////////////
// New property tags
//////////////////////////////////////////////////////////////////
template<class TypeTag, class MyTypeTag>
struct Labels { using type = UndefinedProperty; }; //!< The pore/throat labels

//////////////////////////////////////////////////////////////////
// Property defaults
//////////////////////////////////////////////////////////////////
//! Set the default for the grid geometry
template<class TypeTag>
struct GridGeometry<TypeTag, TTag::PoreNetworkModel>
{
private:
    static constexpr bool enableCache = getPropValue<TypeTag, Properties::EnableGridGeometryCache>();
    using GridView = typename GetPropType<TypeTag, Properties::Grid>::LeafGridView;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
public:
    using type = Dumux::PoreNetwork::GridGeometry<Scalar, GridView, enableCache>;
};

template<class TypeTag>
struct HeatConductionType<TypeTag, TTag::PoreNetworkModel> { using type = Dumux::PoreNetwork::PNMFouriersLaw; };

//! The labels
template<class TypeTag>
struct Labels<TypeTag, TTag::PoreNetworkModel> { using type = Dumux::PoreNetwork::Labels; };

template<class TypeTag>
struct VelocityOutput<TypeTag, TTag::PoreNetworkModel>
{
private:
    using GridVariables = GetPropType<TypeTag, Properties::GridVariables>;
    using FluxVariables = GetPropType<TypeTag, Properties::FluxVariables>;
public:
    using type = Dumux::PoreNetwork::VelocityOutput<GridVariables, FluxVariables>;
};

template<class TypeTag>
struct EnableThermalNonEquilibrium<TypeTag, TTag::PoreNetworkModel> { static constexpr bool value = false; };

// \}
}

} // end namespace

#endif
