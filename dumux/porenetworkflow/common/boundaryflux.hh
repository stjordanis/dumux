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
 * \brief This file contains a class for the calculation of fluxes at the boundary
          of pore-network models.
 *
 */
#ifndef DUMUX_PNM_BOUNDARYFLUX_HH
#define DUMUX_PNM_BOUNDARYFLUX_HH

#include <algorithm>
#include <vector>
#include <dumux/common/typetraits/problem.hh>
#include <dumux/discretization/box/elementboundarytypes.hh>

namespace Dumux::PoreNetwork {

template<class GridVariables, class LocalResidual, class SolutionVector>
class BoundaryFlux
{
    using Problem = std::decay_t<decltype(std::declval<LocalResidual>().problem())>;
    using GridGeometry = typename ProblemTraits<Problem>::GridGeometry;
    using GridView = typename GridGeometry::GridView;
    using Element = typename GridView::template Codim<0>::Entity;
    using FVElementGeometry = typename GridGeometry::LocalView;
    using SubControlVolume = typename FVElementGeometry::SubControlVolume;
    using BoundaryTypes = typename ProblemTraits<Problem>::BoundaryTypes;
    using ElementBoundaryTypes = BoxElementBoundaryTypes<BoundaryTypes>;

    using NumEqVector = typename SolutionVector::block_type;
    static constexpr auto numEq = NumEqVector::dimension;

    //! result struct that holds both the total flux and the flux per pore
    struct Result
    {
        NumEqVector totalFlux;
        std::unordered_map<std::size_t, NumEqVector> fluxPerPore;

        //! make the total flux printable to e.g. std::cout
        friend std::ostream& operator<< (std::ostream& stream, const Result& result)
        {
            stream << result.totalFlux;
            return stream;
        }

        //! allow to get the total flux for a given eqIdx
        const auto& operator[] (int eqIdx) const
        { return totalFlux[eqIdx]; }

        //! make the total flux assignable to NumEqVector through implicit conversion
        operator NumEqVector() const
        { return totalFlux; }
    };

public:
    // export the Scalar type
    using Scalar = typename GridVariables::Scalar;

    BoundaryFlux(const GridVariables& gridVariables,
                 const LocalResidual& localResidual,
                 const SolutionVector& sol)
    : localResidual_(localResidual)
    , gridVariables_(gridVariables)
    , sol_(sol)
    , isStationary_(localResidual.isStationary())
    {
        const auto numDofs = localResidual_.problem().gridGeometry().numDofs();
        isConsidered_.resize(numDofs, false);
        boundaryFluxes_.resize(numDofs);
    }

    /*!
     * \brief Returns the cumulative flux in \f$\mathrm{[\frac{kg}{s}]}\f$ of several pore throats for a given list of pore labels to consider
     *
     * \param labels A list of pore labels which will be considered for the flux calculation
     * \param verbose If set true, the fluxes at all individual SCVs are printed
     */
    template<class Label>
    Result getFlux(const std::vector<Label>& labels, const bool verbose = false) const
    {
        // helper lambda to decide which scvs to consider for flux calculation
        auto restriction = [&] (const SubControlVolume& scv)
        {
            const Label poreLabel = localResidual_.problem().gridGeometry().poreLabel(scv.dofIndex());
            return std::any_of(labels.begin(), labels.end(),
                               [&](const Label l){ return l == poreLabel; });
        };

        std::fill(boundaryFluxes_.begin(), boundaryFluxes_.end(), NumEqVector(0.0));
        std::fill(isConsidered_.begin(), isConsidered_.end(), false);

        // sum up the fluxes
        for (const auto& element : elements(localResidual_.problem().gridGeometry().gridView()))
            getFlux(element, restriction, verbose);

        Result result;
        result.totalFlux = std::accumulate(boundaryFluxes_.begin(), boundaryFluxes_.end(), NumEqVector(0.0));;
        for (int i = 0; i < isConsidered_.size(); ++i)
        {
            if (isConsidered_[i])
                result.fluxPerPore[i] = boundaryFluxes_[i];
        }

        return result;
    }

    /*!
     * \brief Returns the cumulative flux in \f$\mathrm{[\frac{kg}{s}]}\f$ of several pore throats at a given location on the boundary
     *
     * \param minMax Consider bBoxMin or bBoxMax by setting "min" or "max"
     * \param coord x, y or z coordinate at which bBoxMin or bBoxMax is evaluated
     * \param verbose If set true, the fluxes at all individual SCVs are printed
     */
    Result getFlux(const std::string minMax, const int coord, const bool verbose = false) const
    {
        if (!(minMax == "min" || minMax == "max"))
            DUNE_THROW(Dune::InvalidStateException,
                    "second argument must be either 'min' or 'max' (string) !");

        const Scalar eps = 1e-6; //TODO
        auto onMeasuringBoundary = [&] (const Scalar pos)
        {
            return ( (minMax == "min" && pos < localResidual_.problem().gridGeometry().bBoxMin()[coord] + eps) ||
                     (minMax == "max" && pos > localResidual_.problem().gridGeometry().bBoxMax()[coord] - eps) );
        };

        // helper lambda to decide which scvs to consider for flux calculation
        auto restriction = [&] (const SubControlVolume& scv)
        {
            bool considerAllDirections = coord < 0 ? true : false;

            //only consider SCVs on the boundary
            bool considerScv = localResidual_.problem().gridGeometry().dofOnBoundary(scv.dofIndex()) && onMeasuringBoundary(scv.dofPosition()[coord]);

            //check whether a vertex lies on a boundary and also check whether this boundary shall be
            // considered for the flux calculation
            if (considerScv && !considerAllDirections)
            {
                const auto& pos = scv.dofPosition();
                if (!(pos[coord] < localResidual_.problem().gridGeometry().bBoxMin()[coord] + eps || pos[coord] > localResidual_.problem().gridGeometry().bBoxMax()[coord] -eps ))
                considerScv = false;
            }

            return considerScv;
        };

        std::fill(boundaryFluxes_.begin(), boundaryFluxes_.end(), NumEqVector(0.0));
        std::fill(isConsidered_.begin(), isConsidered_.end(), false);

        // sum up the fluxes
        for (const auto& element : elements(localResidual_.problem().gridGeometry().gridView()))
            getFlux(element, restriction, verbose);

        Result result;
        result.totalFlux = std::accumulate(boundaryFluxes_.begin(), boundaryFluxes_.end(), NumEqVector(0.0));;
        for (int i = 0; i < isConsidered_.size(); ++i)
        {
            if (isConsidered_[i])
                result.fluxPerPore[i] = boundaryFluxes_[i];
        }

        return result;

    }

    /*!
     * \brief Returns the cumulative flux in \f$\mathrm{[\frac{kg}{s}]}\f$ of several pore throats at a given location on the boundary
     *
     * \param element The element
     * \param considerScv A lambda function to decide whether to consider a scv or not
     * \param verbose If set true, the fluxes at all individual SCVs are printed
     */
    template<class RestrictingFunction>
    NumEqVector getFlux(const Element& element,
                        RestrictingFunction considerScv,
                        const bool verbose = false) const
    {
        NumEqVector flux(0.0);

        // by default, all coordinate directions are considered for the definition of a boundary

        // make sure FVElementGeometry and volume variables are bound to the element
        auto fvGeometry = localView(localResidual_.problem().gridGeometry());
        fvGeometry.bind(element);

        auto curElemVolVars = localView(gridVariables_.curGridVolVars());
        curElemVolVars.bind(element, fvGeometry, sol_);

        auto prevElemVolVars = localView(gridVariables_.prevGridVolVars());
        if (!isStationary_)
            prevElemVolVars.bindElement(element, fvGeometry, sol_);

        auto elemFluxVarsCache = localView(gridVariables_.gridFluxVarsCache());
        elemFluxVarsCache.bindElement(element, fvGeometry, curElemVolVars);

        ElementBoundaryTypes elemBcTypes;
        elemBcTypes.update(localResidual_.problem(), element, fvGeometry);

        auto residual = localResidual_.evalFluxAndSource(element, fvGeometry, curElemVolVars, elemFluxVarsCache, elemBcTypes);

        if (!isStationary_)
            residual += localResidual_.evalStorage(element, fvGeometry, prevElemVolVars, curElemVolVars);

        for (auto&& scv : scvs(fvGeometry))
        {
            // compute the boundary flux using the local residual of the element's scv on the boundary
            if (considerScv(scv))
            {
                isConsidered_[scv.dofIndex()] = true;

                // The flux must be substracted:
                // On an inlet boundary, the flux part of the local residual will be positive, since all fluxes will leave the SCV towards to interior domain.
                // For the domain itself, however, the sign has to be negative, since mass is entering the system.
                flux -= residual[scv.indexInElement()];

                boundaryFluxes_[scv.dofIndex()] -= residual[scv.indexInElement()];

                if (verbose)
                    std::cout << "SCV of element " << scv.elementIndex()  << " at vertex " << scv.dofIndex() << " has flux: " << residual[scv.indexInElement()] << std::endl;
            }
        }
        return flux;
    }

private:
    const LocalResidual localResidual_; // store a copy of the local residual
    const GridVariables& gridVariables_;
    const SolutionVector& sol_;
    bool isStationary_;
    mutable std::vector<bool> isConsidered_;
    mutable std::vector<NumEqVector> boundaryFluxes_;
};

} // end Dumux::PoreNetwork

#endif
