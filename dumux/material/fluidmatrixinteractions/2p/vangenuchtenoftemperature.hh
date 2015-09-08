// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
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
 *
 * \brief Implementation of the regularized version of the van Genuchten's
 *        capillary pressure / relative permeability  <-> saturation relation
 *        *as function of temperature*.
 */
#ifndef REGULARIZED_VAN_GENUCHTEN_OF_TEMPERATURE_HH
#define REGULARIZED_VAN_GENUCHTEN_OF_TEMPERATURE_HH

#include <dumux/material/fluidmatrixinteractions/2p/regularizedvangenuchten.hh>
#include <dumux/material/fluidmatrixinteractions/2p/regularizedvangenuchtenparams.hh>

namespace Dumux
{
/*!\ingroup fluidmatrixinteractionslaws
 *
 * \brief Implementation of the regularized  van Genuchten's
 *        capillary pressure / relative permeability  <-> saturation relation
 *        *as a function of temperature*.
 *
 *        Everything except the capillary pressure is taken from the parent, i.e. Regularized VanGenuchten.
 */
template <class ScalarT, class ParamsT = RegularizedVanGenuchtenParams<ScalarT> >
class RegularizedVanGenuchtenOfTemperature : public Dumux::RegularizedVanGenuchten<ScalarT, ParamsT>
{
    typedef Dumux::RegularizedVanGenuchten<ScalarT, ParamsT> RegularizedVanGenuchten;
    // Data is in /home/pnuske/paper/pcOfT/

public:
    typedef ParamsT Params;
    typedef typename Params::Scalar Scalar;

    /*!
     * \brief A regularized van Genuchten capillary pressure-saturation
     *          curve *as a function of temperature*.
     *
     *          The standard regularized version of the van Genuchten law is used and subsequantially scaled
     *          by some more or less empirical fit: WRR, Grant(2003)
     *          --> see range of validity (==fit range) in the paper ! <--
     * \param Swe The mobile saturation of the wetting phase.
     * \param params A container object that is populated with the appropriate coefficients for the respective law.
     *                  Therefore, in the (problem specific) spatialParameters  first, the material law is chosen, and then the params container
     *                  is constructed accordingly. Afterwards the values are set there, too.
     * \param temperature The temperature in \f$\mathrm{[K]}\f$
     */
    static Scalar pc(const Params &params, const Scalar & Swe, const Scalar & temperature)
    {
        Scalar beta0    = -413.4 ;
        Scalar TRef     = 298.15 ;
        Scalar pcTemp   = RegularizedVanGenuchten::pc(params,Swe);
        Scalar pc       = pcTemp * ((beta0+temperature) / (beta0+TRef));
        return pc;
    }
};  // class RegularizedVanGenuchtenOfTemperature

}   // namespace Dumux


#endif // REGULARIZED_VAN_GENUCHTEN_OF_TEMPERATURE_HH
