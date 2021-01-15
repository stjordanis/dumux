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
 * \ingroup StaggeredDiscretization
 * \copydoc Dumux::StaggeredFVElementGeometry
 */
#ifndef DUMUX_DISCRETIZATION_FACECENTERED_STAGGERED_FV_ELEMENT_GEOMETRY_HH
#define DUMUX_DISCRETIZATION_FACECENTERED_STAGGERED_FV_ELEMENT_GEOMETRY_HH

#include <type_traits>

#include <dune/common/reservedvector.hh>

#include <dumux/common/indextraits.hh>
#include <dune/common/iteratorrange.hh>
#include <dumux/discretization/scvandscvfiterators.hh>
#include <bitset>

namespace Dumux {

template<class GG, bool cachingEnabled>
class FaceCenteredStaggeredFVElementGeometry;

template<class GG>
class FaceCenteredStaggeredFVElementGeometry<GG, /*cachingEnabled*/true>
{
    using ThisType = FaceCenteredStaggeredFVElementGeometry<GG, /*cachingEnabled*/true>;
    using GridView = typename GG::GridView;
    using GridIndexType = typename IndexTraits<GridView>::GridIndex;

public:
    //! export type of subcontrol volume face
    using SubControlVolume = typename GG::SubControlVolume;
    using SubControlVolumeFace = typename GG::SubControlVolumeFace;
    using Scalar = typename SubControlVolume::Traits::Scalar;
    using Element = typename GridView::template Codim<0>::Entity;
    using GridGeometry = GG;
    using UpwindScheme = typename GridGeometry::UpwindScheme;

    //! the maximum number of scvs per element
    static constexpr auto dim = GridView::dimension;
    static constexpr std::size_t maxNumElementScvs = 2*dim; // (2, 4, 6)
    //! the maximum number of scvfs per element
    static constexpr std::size_t maxNumElementScvfs = maxNumElementScvs * maxNumElementScvs; // (4, 16, 36)

    FaceCenteredStaggeredFVElementGeometry(const GridGeometry& gridGeometry)
    : gridGeometry_(&gridGeometry)
    {}

    const UpwindScheme& staggeredUpwindMethods() const
    { return gridGeometry().staggeredUpwindMethods(); }

    //! Get a sub control volume  with a global scv index
    const SubControlVolume& scv(GridIndexType scvIdx) const
    { return gridGeometry().scv(scvIdx); }

    //! Get a sub control volume face with a global scvf index
    const SubControlVolumeFace& scvf(GridIndexType scvfIdx) const
    { return gridGeometry().scvf(scvfIdx); }

    //! Return the scv in the neighbor element with the same local index
    SubControlVolume nextCorrespondingScv(const SubControlVolume& startScv, const LocalIndexType& targetLocalScvIndex ) const
    {
        assert(!(startScv.boundary()));

        GridIndexType scvIndex = -1;
        auto nextFVGeometry = localView(gridGeometry());
        const auto& nextElement = nextFVGeometry.gridGeometry().element(startScv.neighborElementIdx());
        nextFVGeometry.bind(nextElement);
        for (auto&& neighborElementScv : scvs(nextFVGeometry))
        {
            if ( neighborElementScv.localDofIndex() == targetLocalScvIndex)
                scvIndex = neighborElementScv.index();
        }
        return scv(scvIndex);
    }

    //! Return the frontal sub control volume face on a the boundary for a given sub control volume
    const SubControlVolumeFace& frontalScvfOnBoundary(const SubControlVolume& scv) const
    {
        assert(scv.boundary());

        // frontal boundary faces are always stored after the lateral faces
        auto scvfIter = scvfs(*this, scv).begin();
        const auto end = scvfs(*this, scv).end();
        while (!(scvfIter->isFrontal() && scvfIter->boundary()) && (scvfIter != end))
            ++scvfIter;

        assert(scvfIter->isFrontal());
        assert(scvfIter->boundary());
        return *scvfIter;
    }

    //! Return a the lateral sub control volume face which is orthogonal to the given sub control volume face
    const SubControlVolumeFace& lateralOrthogonalScvf(const SubControlVolumeFace& scvf) const
    {
        assert(scvf.isLateral());
        return gridGeometry().scvf(scvf.scvfIdxWithCommonEntity());
    }




    /////////////////////////
    /// forward neighbors ///
    /////////////////////////

    bool hasForwardNeighbor(const SubControlVolumeFace& frontalScvf) const
    {
        assert(frontalScvf.isFrontal());
        const auto& selfScv = scv(frontalScvf.insideScvIdx());
        return !selfScv.boundary();
    }

    GridIndexType forwardScvIdx(const SubControlVolumeFace& frontalScvf) const
    {
        assert(frontalScvf.isFrontal());
        assert(hasForwardNeighbor(frontalScvf));
        const auto& selfScv = scv(frontalScvf.insideScvIdx());
        const auto& nextScv = nextCorrespondingScv(selfScv, selfScv.indexInElement());
        return nextScv.index();
    }

    //////////////////////////
    /// backward neighbors ///
    //////////////////////////

    bool hasBackwardNeighbor(const SubControlVolumeFace& frontalScvf) const
    {
        assert(frontalScvf.isFrontal());
        const auto& oppositeScv = scv(frontalScvf.outsideScvIdx());
        return !oppositeScv.boundary();
    }

    GridIndexType backwardScvIdx(const SubControlVolumeFace& frontalScvf) const
    {
        assert(frontalScvf.isFrontal());
        assert(hasBackwardNeighbor(frontalScvf));
        const auto& oppositeScv = scv(frontalScvf.outsideScvIdx());
        const auto& nextScv = nextCorrespondingScv(oppositeScv, oppositeScv.indexInElement());
        return nextScv.index();
    }

    /////////////////////////
    /// Frontal Distances ///
    /////////////////////////

    Scalar selfToOppositeDistance(const SubControlVolumeFace& frontalScvf) const
    {
        assert(frontalScvf.isFrontal());
        const auto& selfScv = scv(frontalScvf.insideScvIdx());
        const auto& outsideScv = scv(frontalScvf.outsideScvIdx());
        return (selfScv.dofPosition() - outsideScv.dofPosition()).two_norm();
    }

    Scalar selfToForwardDistance(const SubControlVolumeFace& frontalScvf) const
    {
        assert(frontalScvf.isFrontal());

        const auto& selfScv = scv(frontalScvf.insideScvIdx());
        if (selfScv.boundary())
            return 0.0;

        const auto& forwardScv = nextCorrespondingScv(selfScv, selfScv.indexInElement());
        return (selfScv.dofPosition() - forwardScv.dofPosition()).two_norm();
    }

    Scalar oppositeToBackwardDistance(const SubControlVolumeFace& frontalScvf) const
    {
        assert(frontalScvf.isFrontal());
        const auto& outsideScv = scv(frontalScvf.outsideScvIdx());
        if (outsideScv.boundary())
            return 0.0;

        const auto& backwardScv = nextCorrespondingScv(outsideScv, outsideScv.indexInElement());
        return (outsideScv.dofPosition() - backwardScv.dofPosition()).two_norm();
    }

    //////////////////////////
    /// Parallel Neighbors ///
    //////////////////////////

    bool hasParallelNeighbor(const SubControlVolumeFace& lateralScvf) const
    {
        assert(lateralScvf.isLateral());
        return !lateralScvf.boundary();
    }

    bool hasSecondParallelNeighbor(const SubControlVolumeFace& lateralScvf) const
    {
        assert(lateralScvf.isLateral());
        assert(hasParallelNeighbor(lateralScvf));

        const auto& orthogonalScvf = lateralOrthogonalScvf(lateralScvf);
        const auto& orthagonalScv = scv(orthogonalScvf.insideScvIdx());
        const auto& nextOrthagonalScv = nextCorrespondingScv(orthagonalScv, orthagonalScv.indexInElement());
        return !nextOrthagonalScv.boundary();
    }

    GridIndexType parallelScvIdx(const SubControlVolumeFace& lateralScvf) const
    {
        assert(lateralScvf.isLateral());
        assert(hasParallelNeighbor(lateralScvf));
        return lateralScvf.outsideScvIdx();
    }

    GridIndexType secondParallelScvIdx(const SubControlVolumeFace& lateralScvf) const
    {
        assert(lateralScvf.isLateral());
        assert(hasSecondParallelNeighbor(lateralScvf));

        const auto& selfScv = scv(lateralScvf.insideScvIdx());
        const auto& orthogonalScvf = lateralOrthogonalScvf(lateralScvf);
        const auto& orthagonalScv = scv(orthogonalScvf.insideScvIdx());
        const auto& nextOrthagonalScv = nextCorrespondingScv(orthagonalScv, orthagonalScv.indexInElement());
        const auto& secondParallelScv = nextCorrespondingScv(nextOrthagonalScv, selfScv.indexInElement());
        return secondParallelScv.index();
    }

    SubControlVolumeFace outerParallelLateralScvf(const SubControlVolumeFace& lateralScvf) const
    {
        assert(lateralScvf.isLateral());
        assert(hasParallelNeighbor(lateralScvf));

        const auto& parallelScv = scv(lateralScvf.outsideScvIdx());

        auto fvGeometry = localView(gridGeometry());
        const auto& element = fvGeometry.gridGeometry().element(parallelScv.elementIndex());
        fvGeometry.bind(element);
        GridIndexType index = -1;
        for (auto&& scvf : scvfs(fvGeometry, parallelScv))
        {
            if (scvf.localIndex() == lateralScvf.localIndex())
                index = scvf.index();
        }

        return scvf(index);
    }


    //! iterator range for sub control volumes. Iterates over
    //! all scvs of the bound element (not including neighbor scvs)
    //! This is a free function found by means of ADL
    //! To iterate over all sub control volumes of this FVElementGeometry use
    //! for (auto&& scv : scvs(fvGeometry))
    friend inline auto
    scvs(const FaceCenteredStaggeredFVElementGeometry& fvGeometry)
    {
        using IndexContainerType = std::decay_t<decltype(fvGeometry.scvIndices_())>;
        using ScvIterator = Dumux::ScvIterator<SubControlVolume, IndexContainerType, ThisType>;
        return Dune::IteratorRange<ScvIterator>(ScvIterator(fvGeometry.scvIndices_().begin(), fvGeometry),
                                                ScvIterator(fvGeometry.scvIndices_().end(), fvGeometry));
    }

    //! iterator range for sub control volumes faces. Iterates over
    //! all scvfs of the bound element.
    //! This is a free function found by means of ADL
    //! To iterate over all sub control volume faces of this FVElementGeometry use
    //! for (auto&& scvf : scvfs(fvGeometry))
    friend inline auto
    scvfs(const FaceCenteredStaggeredFVElementGeometry& fvGeometry)
    {
        using IndexContainerType = std::decay_t<decltype(fvGeometry.scvfIndices_())>;
        using ScvfIterator = Dumux::ScvfIterator<SubControlVolumeFace, IndexContainerType, ThisType>;
        return Dune::IteratorRange<ScvfIterator>(ScvfIterator(fvGeometry.scvfIndices_().begin(), fvGeometry),
                                                 ScvfIterator(fvGeometry.scvfIndices_().end(), fvGeometry));
    }

    //! iterator range for sub control volumes faces. Iterates over
    //! all scvfs of the bound element belonging to the given sub control volume.
    //! This is a free function found by means of ADL
    //! To iterate over all sub control volume faces of this FVElementGeometry use
    //! for (auto&& scvf : scvfs(fvGeometry, scv))
    friend inline auto
    scvfs(const FaceCenteredStaggeredFVElementGeometry& fvGeometry, const SubControlVolume& scv)
    {
        using IndexContainerType = std::decay_t<decltype(fvGeometry.scvfIndices_())>;
        // using ScvfIterator = Dumux::ScvfIterator<SubControlVolumeFace, IndexContainerType, ThisType>;
        // const auto localScvIdx = scv.indexInElement();
        // const auto eIdx = fvGeometry.eIdx_;
        // const auto begin = fvGeometry.scvfIndices_().begin() + fvGeometry.gridGeometry().firstLocalScvfIdxOfScv(eIdx, localScvIdx);
        // const auto end = localScvIdx < fvGeometry.numScv()-1 ? fvGeometry.scvfIndices_().begin()
        //                                                        + fvGeometry.gridGeometry().firstLocalScvfIdxOfScv(eIdx, localScvIdx+1)
        //                                                      : fvGeometry.scvfIndices_().end();

        // return Dune::IteratorRange<ScvfIterator>(ScvfIterator(begin, fvGeometry),
        //                                          ScvfIterator(end, fvGeometry));

        using ScvfIterator = Dumux::SkippingScvfIterator<SubControlVolumeFace, IndexContainerType, ThisType>;
        auto begin = ScvfIterator::makeBegin(fvGeometry.scvfIndices_(), fvGeometry, scv.index());
        auto end = ScvfIterator::makeEnd(fvGeometry.scvfIndices_(), fvGeometry, scv.index());
        return Dune::IteratorRange<ScvfIterator>(begin, end);
    }

    //! number of sub control volumes in this fv element geometry
    std::size_t numScv() const
    {
        return scvIndices_().size();
    }

    //! number of sub control volumes in this fv element geometry
    std::size_t numScvf() const
    {
        return scvfIndices_().size();
    }

    //! Returns whether one of the geometry's scvfs lies on a boundary
    bool hasBoundaryScvf() const
    { return gridGeometry().hasBoundaryScvf(eIdx_); }

    //! Binding of an element, called by the local jacobian to prepare element assembly
    void bind(const Element& element)
    {
        this->bindElement(element);
    }

    //! Bind only element-local
    void bindElement(const Element& element)
    {
        elementPtr_ = &element;
        eIdx_ = gridGeometry().elementMapper().index(element);
    }

    //! The grid geometry we are a restriction of
    const GridGeometry& gridGeometry() const
    {
        assert(gridGeometry_);
        return *gridGeometry_;
    }

    std::size_t elementIndex() const
    { return eIdx_; }

private:

    const auto& scvIndices_() const
    {
        return gridGeometry().scvIndicesOfElement(eIdx_);
    }

    const auto& scvfIndices_() const
    {
        return gridGeometry().scvfIndicesOfElement(eIdx_);
    }

    const Element* elementPtr_; //TODO maybe remove
    GridIndexType eIdx_;
    const GridGeometry* gridGeometry_;
};




template<class GG>
class FaceCenteredStaggeredFVElementGeometry<GG, /*cachingEnabled*/false>
{
    using GridView = typename GG::GridView;

    static constexpr std::size_t maxNumScvfs = 16; // TODO 3D

    using Scalar = double; // TODO

    //TODO include assert that checks for quad geometry
    static constexpr auto codimIntersection =  1;
    static constexpr auto dim = GridView::Grid::dimension;

    static constexpr auto numElementFaces = dim * 2;
    static constexpr auto numLateralFacesPerElementFace = 2 * (dim - 1);
    static constexpr auto numLateralFaces = numElementFaces*numLateralFacesPerElementFace;
    static constexpr auto numFacesWithoutRearBoundaryFaces = numLateralFaces + numElementFaces;


    static_assert(numLateralFaces == 8); //TODO remove
    static_assert(numFacesWithoutRearBoundaryFaces == 12); //TODO remove

    using LocalIndexType = typename IndexTraits<GridView>::LocalIndex;

public:
    //! export type of subcontrol volume face
    using SubControlVolume = typename GG::SubControlVolume;
    using SubControlVolumeFace = typename GG::SubControlVolumeFace;
    using Element = typename GridView::template Codim<0>::Entity;
    using GridGeometry = GG;

    FaceCenteredStaggeredFVElementGeometry(const GridGeometry& faceGridGeometry)
    : gridGeometry_(&faceGridGeometry)
    {}

    //! Get a sub control volume face with a local scv index
    const SubControlVolumeFace& scvf(LocalIndexType scvfIdx) const // TODO global scvf idx?
    { return scvfs_[scvfIdx]; }

    //! iterator range for sub control volumes faces. Iterates over
    //! all scvfs of the bound element.
    //! This is a free function found by means of ADL
    //! To iterate over all sub control volume faces of this FVElementGeometry use
    //! for (auto&& scvf : scvfs(fvGeometry))
    friend inline auto
    scvfs(const FaceCenteredStaggeredFVElementGeometry& fvGeometry)
    {
        // using Iter = typename decltype(fvGeometry.scvfs_)::const_iterator;
        // return Dune::IteratorRange<Iter>(fvGeometry.scvfs_.begin(), fvGeometry.scvfs_.end());
    }

    //! Get a half sub control volume with a global scv index
    const SubControlVolume& scv(const std::size_t scvIdx) const
    { return scvs_[findLocalIndex_(scvIdx, globalToLocalScvIdx_)]; }

    //! iterator range for sub control volumes. Iterates over
    //! all scvs of the bound element (not including neighbor scvs)
    //! This is a free function found by means of ADL
    //! To iterate over all sub control volumes of this FVElementGeometry use
    //! for (auto&& scv : scvs(fvGeometry))
    friend inline auto
    scvs(const FaceCenteredStaggeredFVElementGeometry& g)
    {
        using IteratorType = typename std::array<SubControlVolume, 1>::const_iterator;
        return Dune::IteratorRange<IteratorType>(g.scvs_.begin(), g.scvs_.end());
    }

    //! Binding of an element preparing the geometries of the whole stencil
    //! called by the local jacobian to prepare element assembly
    void bind(const Element& element)
    {
        // TODO!

    }

    //! The grid geometry we are a restriction of
    const GridGeometry& gridGeometry() const
    {
        assert(gridGeometry_);
        return *gridGeometry_;
    }

private:

    template<class Entry, class Container>
    const LocalIndexType findLocalIndex_(const Entry& entry,
                                         const Container& container) const
    {
        auto it = std::find(container.begin(), container.end(), entry);
        assert(it != container.end() && "Could not find the entry! Make sure to properly bind this class!");
        return std::distance(container.begin(), it);
    }

    Dune::ReservedVector<SubControlVolumeFace, maxNumScvfs> scvfs_;
    std::array<SubControlVolume, numElementFaces> scvs_;
    std::array<std::size_t, numElementFaces> globalToLocalScvIdx_;

    const GridGeometry* gridGeometry_;
};

} // end namespace

#endif