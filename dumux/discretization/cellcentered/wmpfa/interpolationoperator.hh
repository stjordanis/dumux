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
 * \ingroup CCMpfaDiscretization
 * \brief Base class for interaction volumes of mpfa methods.
 */
#ifndef DUMUX_DISCRETIZATION_CC_MPFA_INTERPOLATIONOPERATOR_HH
#define DUMUX_DISCRETIZATION_CC_MPFA_INTERPOLATIONOPERATOR_HH

#include <vector>

#include <dune/common/exceptions.hh>
#include <dune/geometry/multilineargeometry.hh>

#include "dumux/common/math.hh"

namespace Dumux {


//! Empty interpolator class
class EmptyInterpolator {};

template< class T, bool Enable>
class HapInterpolatorBase
{
    using GridView = typename T::GridView;
    using Element = typename GridView::template Codim<0>::Entity;
    using Position = typename T::GlobalPosition;

    using Scalar = typename Position::value_type;

    struct Entry
    {
        Entry()
        {entry = {-1,0.0};}

        Entry(std::size_t idx, Scalar weight)
        {
            entry = {idx, weight};
        }

        Scalar weight() const
        {return entry.second;}

        std::size_t dofIndex() const
        {return entry.first;}

        private:
            std::pair<std::size_t,Scalar> entry;
    };

    struct LocalInterpolationData
    {
        LocalInterpolationData() = default;

        LocalInterpolationData(Position p, Entry e1, Entry e2)
        {
            position_ = p;
            entries_ = {e1,e2};
        }

        const std::array<Entry, 2>& entries() const
        { return entries_;}

        Position position() const
        {return position_;}

        private:
            Position position_ = {};
            std::array<Entry, 2> entries_ = {Entry(), Entry()};
    };

public:
    //! When global caching is enabled, precompute transmissibilities for all scv faces
    void clear()
    {
        interpolationData_.clear();
        isUpdated_ = false;
    }

    template<class FVElementGeometry, class ElementVolumeVariables, class TF>
    void bind(const typename FVElementGeometry::GridGeometry::GridView::template Codim<0>::Entity& element,
              const FVElementGeometry& fvGeometry,
              const ElementVolumeVariables& elemVolVars,
              const TF& tensor)
    {
        clear();
        interpolationData_.resize(fvGeometry.numScvf());
        for (auto&& scvf : scvfs(fvGeometry))
        {
            const auto insideScvIdx = scvf.insideScvIdx();
            const auto outsideScvIdx = scvf.outsideScvIdx();
            if(!scvf.boundary())
            {
                const auto insideScvIdx = scvf.insideScvIdx();
                const auto outsideScvIdx = scvf.outsideScvIdx();
                const auto& insideVolVars = elemVolVars[insideScvIdx];
                const auto& outsideVolVars = elemVolVars[outsideScvIdx];

                const auto tauK = vtmv(scvf.unitOuterNormal(), tensor(insideVolVars), scvf.unitOuterNormal());
                const auto tauL = vtmv(scvf.unitOuterNormal(), tensor(outsideVolVars), scvf.unitOuterNormal());

                using std::abs;
                const auto centerK = fvGeometry.scv(insideScvIdx).center();
                const auto centerL = fvGeometry.scv(outsideScvIdx).center();
                const auto distK = abs((scvf.center() - fvGeometry.scv(insideScvIdx).center()) * scvf.unitOuterNormal());
                const auto distL = abs((scvf.center() - fvGeometry.scv(outsideScvIdx).center()) * scvf.unitOuterNormal());

                const auto omegaK = distL*tauK / (distL*tauK + distK*tauL);
                const auto omegaL = distK*tauL / (distL*tauK + distK*tauL);

                auto position = (omegaK * centerK) + (omegaL * centerL)
                                + (distL*distK / (distL*tauK + distK*tauL))
                                   * mv(tensor(insideVolVars) - tensor(outsideVolVars), scvf.unitOuterNormal());

                interpolationData_[scvf.localIndex()] = LocalInterpolationData(position, Entry(insideScvIdx, omegaK), Entry(outsideScvIdx, omegaL) );
            }
            else
            {
                interpolationData_[scvf.localIndex()] = LocalInterpolationData(scvf.center(), Entry(insideScvIdx, 0.0), Entry(outsideScvIdx, 1.0) );
            }
        }
        isUpdated_ = true;
    }

    bool isUpdated()
    {
        return isUpdated_;
    }

    template<class FVElementGeometry>
    const std::vector<Position> getDistanceVectors(const FVElementGeometry& fvGeometry) const
    {
        std::vector<Position> distances;
        distances.resize(fvGeometry.numScvf());
        for (auto&& scvf : scvfs(fvGeometry))
            distances[scvf.localIndex()]= (interpolationData_[scvf.localIndex()].position() - fvGeometry.scv(scvf.insideScvIdx()).center());

        return distances;
    }

    template<class FVElementGeometry>
    std::vector<Position> getDistanceVectors(const FVElementGeometry& fvGeometry)
    {
        std::vector<Position> distances;
        distances.resize(fvGeometry.numScvf());
        for (auto&& scvf : scvfs(fvGeometry))
            distances[scvf.localIndex()]= (interpolationData_[scvf.localIndex()].position() - fvGeometry.scv(scvf.insideScvIdx()).center());

        return distances;
    }

    const LocalInterpolationData getInterpolationData(std::size_t localIdx) const
    {
        return interpolationData_[localIdx];
    }

    LocalInterpolationData getInterpolationData(std::size_t localIdx)
    {
        return interpolationData_[localIdx];
    }


private:
    std::vector<LocalInterpolationData> interpolationData_;
    bool isUpdated_;
};

template<class T>
class HapInterpolatorBase<T, false> : public EmptyInterpolator {};

template<class T, class PT>
class HapInterpolationOperator
{

public:
    //! state the traits type publicly
    using Traits = T;
    using GridGeometry = typename T::GridGeometry;
    using GridView = typename T::GridView;
    using Position = typename T::GlobalPosition;
    using Scalar = typename Position::value_type;

    static constexpr bool advectionEnabled = PT::enableAdvection;
    static constexpr bool diffusionEnabled = PT::enableMolecularDiffusion;
    static constexpr bool heatConductionEnabled = PT::enableHeatConduction;

    //! export the underlying process-specific interpolator types
    using AdvectionInterpolator = HapInterpolatorBase<T, advectionEnabled>;
    using DiffusionInterpolator = HapInterpolatorBase<T, diffusionEnabled>;
    using HeatConductionInterpolator = HapInterpolatorBase<T, heatConductionEnabled>;

    //! return references to the interpolator containing data related to advection
    const AdvectionInterpolator& advectionInterpolator() const { return advectionInterpolator_; }
    AdvectionInterpolator& advectionInterpolator() { return advectionInterpolator_; }

    //! return references to the interpolator containing data related to diffusion
    const DiffusionInterpolator& diffusionInterpolator() const { return diffusionInterpolator_; }
    DiffusionInterpolator& diffusionInterpolator() { return diffusionInterpolator_; }

    //! return references to the interpolator containing data related to heat conduction
    const HeatConductionInterpolator& heatConductionInterpolator() const { return heatConductionInterpolator_; }
    HeatConductionInterpolator& heatConductionInterpolator() { return heatConductionInterpolator_; }

    template<class FVElementGeometry, class ElementVolumeVariables>
    void bind(const typename FVElementGeometry::GridGeometry::GridView::template Codim<0>::Entity& element,
              const FVElementGeometry& fvGeometry,
              const ElementVolumeVariables& elemVolVars)
    {
        // (maybe) solve system subject to intrinsic permeability
        if constexpr (advectionEnabled)
        {
            auto getTensor = [] (const auto& volVars) { return volVars.permeability(); };
            advectionInterpolator_.bind(element, fvGeometry, elemVolVars, getTensor);
        }

        // (maybe) solve system subject to diffusion tensors
        if constexpr (diffusionEnabled)
        {
        }

        // (maybe) solve system subject to thermal conductivity
        if constexpr (heatConductionEnabled)
        {
        }
    }

private:
    AdvectionInterpolator advectionInterpolator_;
    DiffusionInterpolator diffusionInterpolator_;
    HeatConductionInterpolator heatConductionInterpolator_;
};

} // end namespace Dumux

#endif