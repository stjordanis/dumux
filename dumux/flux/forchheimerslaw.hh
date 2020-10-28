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
 * \ingroup Flux
 * \brief Forchheimer's law specialized for different discretization schemes
 *        This file contains the data which is required to calculate
 *        volume and mass fluxes of fluid phases over a face of a finite volume by means
 *        of the Forchheimer approximation. Specializations are provided for the different discretization methods.
 */
#ifndef DUMUX_DISCRETIZATION_FORCHHEIMERS_LAW_HH
#define DUMUX_DISCRETIZATION_FORCHHEIMERS_LAW_HH

#include <dumux/common/properties.hh>
#include <dumux/discretization/method.hh>
#include "forchheimervelocity.hh"

namespace Dumux {

// forward declaration
template <class TypeTag, class VelocityLaw, DiscretizationMethod discMethod>
class ForchheimersLawImplementation
{
    static_assert(discMethod == DiscretizationMethod::cctpfa || discMethod == DiscretizationMethod::box, "Forchheimer only implemented for cctpfa or box!");
};

/*!
 * \ingroup Flux
 * \brief Evaluates the normal component of the Forchheimer velocity on a (sub)control volume face.
 * \note Specializations are provided for the different discretization methods.
 * These specializations are found in the headers included below.
 */
template <class TypeTag>
using ForchheimersLaw = ForchheimersLawImplementation<TypeTag,
                                                      ForchheimerVelocity<GetPropType<TypeTag, Properties::Scalar>, GetPropType<TypeTag, Properties::GridGeometry>>,
                                                      GetPropType<TypeTag, Properties::GridGeometry>::discMethod>;

} // end namespace Dumux

#include <dumux/flux/cctpfa/forchheimerslaw.hh>
//#include <dumux/flux/box/forchheimerslaw.hh>

#endif
