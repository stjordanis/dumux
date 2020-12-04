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
 * \ingroup DiamondDiscretization
 * \copydoc Dumux::FaceCenteredDiamondElementVolumeVariables
 */
#ifndef DUMUX_DISCRETIZATION_FACECENTERED_DIAMOND_ELEMENTVOLUMEVARIABLES_HH
#define DUMUX_DISCRETIZATION_FACECENTERED_DIAMOND_ELEMENTVOLUMEVARIABLES_HH

#include <algorithm>
#include <cassert>
#include <vector>

namespace Dumux {

/*!
 * \ingroup DiamondDiscretization
 * \brief Base class for the face variables vector
 */
template<class GridVolumeVariables, bool cachingEnabled>
class FaceCenteredDiamondElementVolumeVariables
{};

/*!
 * \ingroup DiamondDiscretization
 * \brief Class for the face variables vector. Specialization for the case of storing the face variables globally.
 */
template<class GFV>
class FaceCenteredDiamondElementVolumeVariables<GFV, /*cachingEnabled*/true>
{
public:
    //! export type of the grid volume variables
    using GridVolumeVariables = GFV;

    //! export type of the volume variables
    using VolumeVariables = typename GridVolumeVariables::VolumeVariables;

    FaceCenteredDiamondElementVolumeVariables(const GridVolumeVariables& gridVolumeVariables)
    : gridVolumeVariablesPtr_(&gridVolumeVariables)
    , numScv_(gridVolumeVariables.problem().gridGeometry().numScv())
    {}

    //! operator for the access with an scvf
    template<class SubControlVolume, typename std::enable_if_t<!std::is_integral<SubControlVolume>::value, int> = 0>
    const VolumeVariables& operator [](const SubControlVolume& scv) const
    {
        if (scv.index() < numScv_)
            return gridVolVars().volVars(scv.index());
        else
            DUNE_THROW(Dune::InvalidStateException, "Scv index out of bounds");
    }

    //! operator for the access with an index
    //! needed for cc methods for the access to the boundary volume variables
    const VolumeVariables& operator [](const std::size_t scvIdx) const
    {
        if (scvIdx < numScv_)
            return gridVolVars().volVars(scvIdx);
        else
            DUNE_THROW(Dune::InvalidStateException, "Scv index out of bounds");
    }


    //! For compatibility reasons with the case of not storing the face vars.
    //! function to be called before assembling an element, preparing the vol vars within the stencil
    template<class FVElementGeometry, class SolutionVector>
    void bind(const typename FVElementGeometry::GridGeometry::GridView::template Codim<0>::Entity& element,
              const FVElementGeometry& fvGeometry,
              const SolutionVector& sol)
    {
        // if (!fvGeometry.hasBoundaryScvf())
        //     return;

        // clear_();
        // boundaryVolVarIndices_.reserve(fvGeometry.gridGeometry().numBoundaryScv());
        // boundaryVolumeVariables_.reserve(fvGeometry.gridGeometry().numBoundaryScv());

        // for (const auto& scvf : scvfs(fvGeometry))
        // {
        //     if (!scvf.boundary())
        //         continue;

        //     // check if boundary is a pure dirichlet boundary
        //     const auto& problem = gridVolVars().problem();
        //     const auto bcTypes = problem.boundaryTypes(element, scvf);

        //     auto addBoundaryVolVars = [&](const auto& scvFace)
        //     {
        //         const auto& scvI = fvGeometry.scv(scvFace.insideScvIdx());
        //         typename VolumeVariables::PrimaryVariables pv(problem.dirichlet(element, scvFace)[scvI.directionIndex()]);
        //         const auto dirichletPriVars = elementSolution<FVElementGeometry>(element, pv, fvGeometry);

        //         VolumeVariables volVars;
        //         volVars.update(dirichletPriVars,
        //                        problem,
        //                        element,
        //                        scvI);

        //         boundaryVolumeVariables_.emplace_back(std::move(volVars));
        //         boundaryVolVarIndices_.push_back(scvFace.outsideScvIdx());
        //     };

        //     if (bcTypes.hasOnlyDirichlet())
        //     {
        //         addBoundaryVolVars(scvf);
        //         continue;
        //     }
        // }
        // assert(boundaryVolumeVariables_.size() == boundaryVolVarIndices_.size());
    }

    //! Binding of an element, prepares only the face variables of the element
    //! specialization for Diamond models
    template<class FVElementGeometry, class SolutionVector>
    void bindElement(const typename FVElementGeometry::GridGeometry::GridView::template Codim<0>::Entity& element,
                     const FVElementGeometry& fvGeometry,
                     const SolutionVector& sol)
    {}


    //! The global volume variables object we are a restriction of
    const GridVolumeVariables& gridVolVars() const
    { return *gridVolumeVariablesPtr_; }


private:
    //! Clear all local storage
    void clear_()
    {
        // boundaryVolVarIndices_.clear();
        // boundaryVolumeVariables_.clear();
    }

    // //! map a global scv index to the local storage index
    // int getLocalIdx_(const std::size_t volVarIdx) const
    // {
    //     auto it = std::find(boundaryVolVarIndices_.begin(), boundaryVolVarIndices_.end(), volVarIdx);

    //     if (it == boundaryVolVarIndices_.end())
    //     {
    //         std::cout << "volVarIdx " << volVarIdx << std::endl;
    //         for (const auto i : boundaryVolVarIndices_)
    //             std::cout << i << std::endl;
    //     }

    //     assert(it != boundaryVolVarIndices_.end() && "Could not find the current volume variables for volVarIdx!");
    //     return std::distance(boundaryVolVarIndices_.begin(), it);
    // }

    const GridVolumeVariables* gridVolumeVariablesPtr_;
    const std::size_t numScv_;
    // std::vector<std::size_t> boundaryVolVarIndices_;
    // std::vector<VolumeVariables> boundaryVolumeVariables_;
};

/*!
 * \ingroup DiamondDiscretization
 * \brief Class for the face variables vector. Specialization for the case of not storing the face variables globally.
 */
template<class GFV>
class FaceCenteredDiamondElementVolumeVariables<GFV, /*cachingEnabled*/false>
{
public:
    //! export type of the grid volume variables
    using GridVolumeVariables = GFV;

    //! export type of the volume variables
    using VolumeVariables = typename GridVolumeVariables::VolumeVariables;

    FaceCenteredDiamondElementVolumeVariables(const GridVolumeVariables& globalFacesVars) : gridVolumeVariablesPtr_(&globalFacesVars) {}

    //! const operator for the access with an scvf
    template<class SubControlVolume, typename std::enable_if_t<!std::is_integral<SubControlVolume>::value, int> = 0>
    const VolumeVariables& operator [](const SubControlVolume& scv) const
    { return faceVariables_[scv.indexInElement()]; }

    //! const operator for the access with an index
    const VolumeVariables& operator [](const std::size_t scvIdx) const
    { return faceVariables_[getLocalIdx_(scvIdx)]; }

    //! operator for the access with an scvf
    template<class SubControlVolume, typename std::enable_if_t<!std::is_integral<SubControlVolume>::value, int> = 0>
    VolumeVariables& operator [](const SubControlVolume& scv)
    { return faceVariables_[scv.indexInElement()]; }

    // operator for the access with an index
    VolumeVariables& operator [](const std::size_t scvIdx)
    { return faceVariables_[getLocalIdx_(scvIdx)]; }

    //! For compatibility reasons with the case of not storing the vol vars.
    //! function to be called before assembling an element, preparing the vol vars within the stencil
    template<class FVElementGeometry, class SolutionVector>
    void bind(const typename FVElementGeometry::GridGeometry::GridView::template Codim<0>::Entity& element,
              const FVElementGeometry& fvGeometry,
              const SolutionVector& sol)
    {
        // TODO
        assert(false);

    }

    //! Binding of an element, prepares only the face variables of the element
    //! specialization for Diamond models
    template<class FVElementGeometry, class SolutionVector>
    void bindElement(const typename FVElementGeometry::GridGeometry::GridView::template Codim<0>::Entity& element,
                     const FVElementGeometry& fvGeometry,
                     const SolutionVector& sol)
    {
        // TODO
        assert(false);
    }
    //! The global volume variables object we are a restriction of
    const GridVolumeVariables& gridVolumeVariables() const
    { return *gridVolumeVariablesPtr_; }

private:

    int getLocalIdx_(const int scvfIdx) const
    {
        auto it = std::find(faceVarIndices_.begin(), faceVarIndices_.end(), scvfIdx);
        assert(it != faceVarIndices_.end() && "Could not find the current face variables for scvfIdx!");
        return std::distance(faceVarIndices_.begin(), it);
    }

    const GridVolumeVariables* gridVolumeVariablesPtr_;
    std::vector<std::size_t> faceVarIndices_;
    std::vector<VolumeVariables> faceVariables_;
};

} // end namespace

#endif
