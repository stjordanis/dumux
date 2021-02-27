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
 * \brief The properties for the Darcy sub-problem of the couple Stokes-Darcy convergence test.
 */
#ifndef DUMUX_DARCY_SUB_PROPERTIES_HH
#define DUMUX_DARCY_SUB_PROPERTIES_HH

#include <dune/grid/yaspgrid.hh>

#include <dumux/discretization/cctpfa.hh>

#include <dumux/material/components/constant.hh>
#include <dumux/material/fluidsystems/1pliquid.hh>

#include <dumux/porousmediumflow/1p/model.hh>
#include <dumux/porousmediumflow/problem.hh>

#include "spatialparams.hh"

#include "problem_darcy.hh"

namespace Dumux::Properties {
// Create new type tags
namespace TTag {
struct DarcyOneP { using InheritsFrom = std::tuple<OneP, CCTpfaModel>; };
} // end namespace TTag

// Set the problem property
template<class TypeTag>
struct Problem<TypeTag, TTag::DarcyOneP> { using type = Dumux::DarcySubProblem<TypeTag>; };

// the fluid system
template<class TypeTag>
struct FluidSystem<TypeTag, TTag::DarcyOneP>
{
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = FluidSystems::OnePLiquid<Scalar, Dumux::Components::Constant<1, Scalar> > ;
};

// Set the grid type
template<class TypeTag>
struct Grid<TypeTag, TTag::DarcyOneP> { using type = Dune::YaspGrid<2>; };

template<class TypeTag>
struct SpatialParams<TypeTag, TTag::DarcyOneP>
{
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using type = ConvergenceTestSpatialParams<GridGeometry, Scalar>;
};
} // end namespace Dumux::Properties

#endif
