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
 * \ingroup NavierStokesModel
 * \copydoc Dumux::NavierStokesMassOnePFluxVariables
 */
#ifndef DUMUX_NAVIERSTOKES_MASS_1P_FLUXVARIABLES_HH
#define DUMUX_NAVIERSTOKES_MASS_1P_FLUXVARIABLES_HH

#include <dumux/common/properties.hh>
#include <dumux/common/exceptions.hh>
#include <dumux/common/parameters.hh>
#include <dumux/flux/upwindscheme.hh>
#include <dumux/freeflow/navierstokes/scalarvolumevariables.hh>
#include <dumux/freeflow/navierstokes/scalarfluxvariables.hh>

namespace Dumux {

/*!
 * \ingroup NavierStokesModel
 * \brief The flux variables class for the single-phase flow Navier-Stokes model.
 */
template<class Problem,
         class ModelTraits,
         class FluxTs,
         class ElementVolumeVariables,
         class ElementFluxVariablesCache,
         class UpwindScheme = UpwindScheme<typename ProblemTraits<Problem>::GridGeometry>>
class NavierStokesMassOnePFluxVariables
: public NavierStokesScalarConservationModelFluxVariables<Problem,
                                                          ModelTraits,
                                                          FluxTs,
                                                          ElementVolumeVariables,
                                                          ElementFluxVariablesCache,
                                                          UpwindScheme>
{
    using ParentType = NavierStokesScalarConservationModelFluxVariables<Problem,
                                                                        ModelTraits,
                                                                        FluxTs,
                                                                        ElementVolumeVariables,
                                                                        ElementFluxVariablesCache,
                                                                        UpwindScheme>;

    using VolumeVariables = typename ElementVolumeVariables::VolumeVariables;
    using NumEqVector = typename VolumeVariables::PrimaryVariables;

public:

    /*!
     * \brief Returns the advective mass flux in kg/s.
     */
    NumEqVector advectiveFlux(int phaseIdx = 0) const
    {
        auto upwindTerm = [](const VolumeVariables& volVars) { return volVars.density(); };
        return NumEqVector{ParentType::advectiveFlux(upwindTerm)};
    }

    /*!
     * \brief Returns all fluxes for the single-phase flow Navier-Stokes model: the
     *        advective mass flux in kg/s and the energy flux in J/s (for nonisothermal models).
     */
    NumEqVector flux(int phaseIdx = 0) const
    {
        NumEqVector flux = advectiveFlux(phaseIdx);
        ParentType::addHeatFlux(flux);
        return flux;
    }
};

} // end namespace Dumux

#endif
