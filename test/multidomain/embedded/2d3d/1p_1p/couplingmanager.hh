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
 * \ingroup EmbeddedCoupling
 * \ingroup CCModel
 * \brief Coupling manager for low-dimensional domains embedded in the bulk
 *        domain. Point sources on each integration point are computed by an AABB tree.
 *        Both domain are assumed to be discretized using a cc finite volume scheme.
 */

#ifndef DUMUX_CC_BBOXTREE_EMBEDDEDCOUPLINGMANAGER_FRACTURE_HH
#define DUMUX_CC_BBOXTREE_EMBEDDEDCOUPLINGMANAGER_FRACTURE_HH

#include <iostream>
#include <fstream>
#include <string>
#include <utility>

#include <dune/common/timer.hh>
#include <dune/geometry/quadraturerules.hh>
#include <dune/geometry/referenceelements.hh>

#include <dumux/common/properties.hh>
#include <dumux/multidomain/couplingmanager.hh>
#include <dumux/mixeddimension/glue/glue.hh>
#include <dumux/mixeddimension/embedded/cellcentered/pointsourcedata.hh>

namespace Dumux {

/*!
 * \ingroup MixedDimension
 * \ingroup MixedDimensionEmbedded
 * \brief Manages the coupling between bulk elements and lower dimensional elements
 *        Point sources on each integration point are computed by an AABB tree.
 *        Both domain are assumed to be discretized using a cc finite volume scheme.
 */
template<class MDTraits>
class EmbeddedFractureCouplingManager
: public CouplingManager<MDTraits>
{
    using ParentType = CouplingManager<MDTraits>;
    using Scalar = typename MDTraits::Scalar;
    static constexpr auto bulkIdx = typename MDTraits::template DomainIdx<0>();
    static constexpr auto lowDimIdx = typename MDTraits::template DomainIdx<1>();
    using SolutionVector = typename MDTraits::SolutionVector;
    using PointSourceData = Dumux::PointSourceData<MDTraits>;
    using CouplingStencils = std::unordered_map<std::size_t, std::vector<std::size_t> >;
    using CouplingStencil = CouplingStencils::mapped_type;

    // the sub domain type tags
    template<std::size_t id>
    using SubDomainTypeTag = typename MDTraits::template SubDomainTypeTag<id>;

    template<std::size_t id> using Problem = typename GET_PROP_TYPE(SubDomainTypeTag<id>, Problem);
    template<std::size_t id> using PointSource = typename GET_PROP_TYPE(SubDomainTypeTag<id>, PointSource);
    template<std::size_t id> using PrimaryVariables = typename GET_PROP_TYPE(SubDomainTypeTag<id>, PrimaryVariables);
    template<std::size_t id> using NumEqVector = typename GET_PROP_TYPE(SubDomainTypeTag<id>, NumEqVector);
    template<std::size_t id> using VolumeVariables = typename GET_PROP_TYPE(SubDomainTypeTag<id>, VolumeVariables);
    template<std::size_t id> using ElementVolumeVariables = typename GET_PROP_TYPE(SubDomainTypeTag<id>, GridVolumeVariables)::LocalView;
    template<std::size_t id> using FVGridGeometry = typename GET_PROP_TYPE(SubDomainTypeTag<id>, FVGridGeometry);
    template<std::size_t id> using GridView = typename FVGridGeometry<id>::GridView;
    template<std::size_t id> using FVElementGeometry = typename FVGridGeometry<id>::LocalView;
    template<std::size_t id> using ElementBoundaryTypes = typename GET_PROP_TYPE(SubDomainTypeTag<id>, ElementBoundaryTypes);
    template<std::size_t id> using ElementFluxVariablesCache = typename GET_PROP_TYPE(SubDomainTypeTag<id>, GridFluxVariablesCache)::LocalView;
    template<std::size_t id> using Element = typename GridView<id>::template Codim<0>::Entity;

    enum {
        bulkDim = GridView<bulkIdx>::dimension,
        lowDimDim = GridView<lowDimIdx>::dimension,
        dimWorld = GridView<bulkIdx>::dimensionworld
    };

    using GlueType = CCMixedDimensionGlue<GridView<bulkIdx>, GridView<lowDimIdx>>;

public:

    /*!
     * \brief Constructor
     */
    EmbeddedFractureCouplingManager(std::shared_ptr<const FVGridGeometry<bulkIdx>> bulkFvGridGeometry,
                                    std::shared_ptr<const FVGridGeometry<lowDimIdx>> lowDimFvGridGeometry)
    {
        glue_ = std::make_shared<GlueType>();
        computePointSourceData(*bulkFvGridGeometry, *lowDimFvGridGeometry);
    }

    /*!
     * \brief Methods to be accessed by main
     */
    // \{

    void init(std::shared_ptr<Problem<bulkIdx>> bulkProblem,
              std::shared_ptr<Problem<lowDimIdx>> lowDimProblem,
              const SolutionVector& curSol)
    {
        curSol_ = curSol;
        problemTuple_ = std::make_tuple(bulkProblem, lowDimProblem);
    }

    /*!
     * \brief Update after the grid has changed
     */
    void update()
    {
        computePointSourceData();
    }

    /*!
     * \brief Update the solution vector before assembly
     */
    void updateSolution(const SolutionVector& curSol)
    { curSol_ = curSol; }

    // \}

    /*!
     * \brief Methods to be accessed by the assembly
     */
    // \{

    /*!
     * \brief The coupling stencil of domain I, i.e. which domain J dofs
     *        the given domain I element's residual depends on.
     */
    template<std::size_t i, std::size_t j>
    const CouplingStencil& couplingElementStencil(const Element<i>& element,
                                                  Dune::index_constant<i> domainI,
                                                  Dune::index_constant<j> domainJ) const
    {
        static_assert(i != j, "A domain cannot be coupled to itself!");

        const auto eIdx = problem(domainI).fvGridGeometry().elementMapper().index(element);
        if (couplingStencils(domainI).count(eIdx))
            return couplingStencils(domainI).at(eIdx);
        else
            return emptyStencil_;
    }

    //! the local and global dof indices of the coupled element with index globalJ coupling to dofs of elementI
    //! for the cellcentered scheme there is only one dof in the element center
    template<std::size_t i, std::size_t j, class IndexTypeJ>
    auto coupledElementDofData(Dune::index_constant<i> domainI,
                               const Element<i>& elementI,
                               Dune::index_constant<j> domainJ,
                               IndexTypeJ globalJ) const
    { return std::array<typename ParentType::template DofData<i,j>, 1>({{globalJ, 0}}); }

    //! evaluate coupling residual for the derivative residual i with respect to privars of dof j
    //! we only need to evaluate the part of the residual that will be influenced by the the privars of dof j
    //! i.e. the source term.
    //! the coupling residual is symmetric so we only need to one template function here
    template<std::size_t i, std::size_t j, class LocalResidualI>
    auto evalCouplingResidual(Dune::index_constant<i> domainI,
                              const Element<i>& elementI,
                              const FVElementGeometry<i>& fvGeometry,
                              const ElementVolumeVariables<i>& curElemVolVars,
                              const ElementBoundaryTypes<i>& elemBcTypes,
                              const ElementFluxVariablesCache<i>& elemFluxVarsCache,
                              const LocalResidualI& localResidual,
                              Dune::index_constant<j> domainJ,
                              const Element<j>& elementJ)
    -> typename LocalResidualI::ElementResidualVector
    {
        static_assert(i != j, "A domain cannot be coupled to itself!");

        typename LocalResidualI::ElementResidualVector residual;
        residual.resize(fvGeometry.numScv());
        for (const auto& scv : scvs(fvGeometry))
        {
            auto couplingSource = problem(domainI).scvPointSources(elementI, fvGeometry, curElemVolVars, scv);
            couplingSource *= -scv.volume()*curElemVolVars[scv].extrusionFactor();
            residual[scv.indexInElement()] = couplingSource;
        }
        return residual;
    }

    /*!
     * \brief Bind the coupling context
     */
    template<class Element, std::size_t i, class Assembler>
    void bindCouplingContext(Dune::index_constant<i> domainI, const Element& element, const Assembler& assembler)
    {}

    /*!
     * \brief Update the coupling context for a derivative i->j
     */
    template<std::size_t i, std::size_t j, class Assembler, class ElemSolJ>
    void updateCouplingContext(Dune::index_constant<i> domainI,
                               Dune::index_constant<j> domainJ,
                               const Element<j>& element,
                               const ElemSolJ& elemSol,
                               std::size_t localDofIdx,
                               std::size_t pvIdx,
                               const Assembler& assembler)
    {
        const auto eIdx = problem(domainJ).fvGridGeometry().elementMapper().index(element);
        curSol_[domainJ][eIdx][pvIdx] = elemSol[localDofIdx][pvIdx];
    }

    // \}

    /*!
     * \brief Main update routine
     */
    // \{

    /* \brief Compute integration point point sources and associated data
     *
     * This method uses grid glue to intersect the given grids. Over each intersection
     * we later need to integrate a source term. This method places point sources
     * at each quadrature point and provides the point source with the necessary
     * information to compute integrals (quadrature weight and integration element)
     * \param order The order of the quadrature rule for integration of sources over an intersection
     * \param verbose If the point source computation is verbose
     */
    void computePointSourceData(const FVGridGeometry<bulkIdx>& bulkFvGridGeometry,
                                const FVGridGeometry<lowDimIdx>& lowDimFvGridGeometry,
                                std::size_t order = 1,
                                bool verbose = false)
    {
        // initilize the maps
        // do some logging and profiling
        Dune::Timer watch;
        std::cout << "Initializing the point sources..." << std::endl;

        // clear all internal members like pointsource vectors and stencils
        // initializes the point source id counter
        clear();

        // intersect the bounding box trees
        glue_->build(bulkFvGridGeometry.boundingBoxTree(),
                     lowDimFvGridGeometry.boundingBoxTree());

        pointSourceData_.reserve(glue_->size());
        averageDistanceToBulkCC_.reserve(glue_->size());
        for (const auto& is : intersections(*glue_))
        {
            // all inside elements are identical...
            const auto& inside = is.inside(0);
            // get the intersection geometry for integrating over it
            const auto intersectionGeometry = is.geometry();

            // get the Gaussian quadrature rule for the local intersection
            const auto& quad = Dune::QuadratureRules<Scalar, lowDimDim>::rule(intersectionGeometry.type(), order);
            const std::size_t lowDimElementIdx = lowDimFvGridGeometry.elementMapper().index(inside);

            // iterate over all quadrature points
            for (auto&& qp : quad)
            {
                // compute the coupling stencils
                for (std::size_t outsideIdx = 0; outsideIdx < is.neighbor(0); ++outsideIdx)
                {
                    const auto& outside = is.outside(outsideIdx);
                    const std::size_t bulkElementIdx = bulkFvGridGeometry.elementMapper().index(outside);

                    // each quadrature point will be a point source for the sub problem
                    const auto globalPos = intersectionGeometry.global(qp.position());
                    const auto id = idCounter_++;
                    const auto qpweight = qp.weight();
                    const auto ie = intersectionGeometry.integrationElement(qp.position());
                    bulkPointSources_.emplace_back(globalPos, id, qpweight, ie, std::vector<std::size_t>({bulkElementIdx}));
                    bulkPointSources_.back().setEmbeddings(is.neighbor(0));
                    lowDimPointSources_.emplace_back(globalPos, id, qpweight, ie, std::vector<std::size_t>({lowDimElementIdx}));
                    lowDimPointSources_.back().setEmbeddings(is.neighbor(0));

                    // pre compute additional data used for the evaluation of
                    // the actual solution dependent source term
                    PointSourceData psData;
                    psData.addLowDimInterpolation(lowDimElementIdx);
                    psData.addBulkInterpolation(bulkElementIdx);

                    // publish point source data in the global vector
                    pointSourceData_.emplace_back(std::move(psData));
                    averageDistanceToBulkCC_.push_back(computeDistance_(outside.geometry(), globalPos));

                    // compute the coupling stencils
                    bulkCouplingStencils_[bulkElementIdx].push_back(lowDimElementIdx);
                    // add this bulk element to the low dim coupling stencil
                    lowDimCouplingStencils_[lowDimElementIdx].push_back(bulkElementIdx);
                }
            }
        }

        // coupling stencils
        for (auto&& stencil : bulkCouplingStencils_)
        {
            std::sort(stencil.second.begin(), stencil.second.end());
            stencil.second.erase(std::unique(stencil.second.begin(), stencil.second.end()), stencil.second.end());
        }
        // the low dim coupling stencil
        for (auto&& stencil : lowDimCouplingStencils_)
        {
            std::sort(stencil.second.begin(), stencil.second.end());
            stencil.second.erase(std::unique(stencil.second.begin(), stencil.second.end()), stencil.second.end());
        }

        std::cout << "took " << watch.elapsed() << " seconds." << std::endl;
    }

    // \}

    /*!
     * \brief Methods to be accessed by the subproblems
     */
    // \{

    //! Return a reference to the pointSource data
    const PointSourceData& pointSourceData(std::size_t id) const
    {
        return pointSourceData_[id];
    }

    //! Return a reference to the pointSource data
    Scalar averageDistance(std::size_t id) const
    {
        return averageDistanceToBulkCC_[id];
    }

    //! Return a reference to the bulk problem
    template<std::size_t id>
    const Problem<id>& problem(Dune::index_constant<id> domainIdx) const
    {
        return *std::get<id>(problemTuple_);
    }

    //! Return a reference to the bulk problem
    template<std::size_t id>
    Problem<id>& problem(Dune::index_constant<id> domainIdx)
    {
        return *std::get<id>(problemTuple_);
    }

    //! Return a reference to the bulk problem
    template<std::size_t id>
    const GridView<id>& gridView(Dune::index_constant<id> domainIdx) const
    {
        return problem(domainIdx).fvGridGeometry().gridView();
    }

    //! Return data for a bulk point source with the identifier id
    PrimaryVariables<bulkIdx> bulkPriVars(std::size_t id) const
    {
        auto& data = pointSourceData_[id];
        return data.interpolateBulk(curSol_[bulkIdx]);
    }

    //! Return data for a low dim point source with the identifier id
    PrimaryVariables<lowDimIdx> lowDimPriVars(std::size_t id) const
    {
        auto& data = pointSourceData_[id];
        return data.interpolateLowDim(curSol_[lowDimIdx]);
    }

    //! Return reference to bulk point sources
    const std::vector<PointSource<bulkIdx>>& bulkPointSources() const
    { return bulkPointSources_; }

    //! Return reference to low dim point sources
    const std::vector<PointSource<lowDimIdx>>& lowDimPointSources() const
    { return lowDimPointSources_; }

    //! Return reference to bulk coupling stencil member
    template<std::size_t id>
    const CouplingStencils& couplingStencils(Dune::index_constant<id> dom) const
    { return (id == bulkIdx) ? bulkCouplingStencils_ : lowDimCouplingStencils_; }

    //! Clear all internal data members
    void clear()
    {
        bulkPointSources_.clear();
        lowDimPointSources_.clear();
        pointSourceData_.clear();
        bulkCouplingStencils_.clear();
        lowDimCouplingStencils_.clear();
        idCounter_ = 0;
    }

    template<std::size_t i>
    const std::vector<std::size_t>& getAdditionalDofDependencies(Dune::index_constant<i> dom, std::size_t eIdx) const
    { return emptyStencil_; }

    template<std::size_t i>
    const std::vector<std::size_t>& getAdditionalDofDependenciesInverse(Dune::index_constant<i> dom, std::size_t eIdx) const
    { return emptyStencil_; }

protected:
    //! Return reference to point source data vector member
    std::vector<PointSourceData>& pointSourceData()
    { return pointSourceData_; }

    //! Return reference to bulk point sources
    std::vector<PointSource<bulkIdx>>& bulkPointSources()
    { return bulkPointSources_; }

    //! Return reference to low dim point sources
    std::vector<PointSource<lowDimIdx>>& lowDimPointSources()
    { return lowDimPointSources_; }

    //! Return reference to bulk coupling stencil member
    template<std::size_t id>
    CouplingStencils& couplingStencils(Dune::index_constant<id> dom)
    { return (id == 0) ? bulkCouplingStencils_ : lowDimCouplingStencils_; }

    //! Return a reference to an empty stencil
    const CouplingStencil& emptyStencil() const
    { return emptyStencil_; }

private:

    template<class Geometry, class GlobalPosition>
    Scalar computeDistance_(const Geometry& geometry, const GlobalPosition& p)
    {
        Scalar avgDist = 0.0;
        const auto& quad = Dune::QuadratureRules<Scalar, bulkDim>::rule(geometry.type(), 5);
        for (auto&& qp : quad)
        {
            const auto globalPos = geometry.global(qp.position());
            avgDist += (globalPos-p).two_norm()*qp.weight();
        }
        return avgDist;
    }

    std::tuple<std::shared_ptr<Problem<0>>, std::shared_ptr<Problem<1>>> problemTuple_;

    std::vector<PointSource<bulkIdx>> bulkPointSources_;
    std::vector<PointSource<lowDimIdx>> lowDimPointSources_;

    mutable std::vector<PointSourceData> pointSourceData_;
    std::vector<Scalar> averageDistanceToBulkCC_;

    CouplingStencils bulkCouplingStencils_;
    CouplingStencils lowDimCouplingStencils_;
    CouplingStencil emptyStencil_;

    //! id generator for point sources
    std::size_t idCounter_ = 0;

    //! The glue object
    std::shared_ptr<GlueType> glue_;

    ////////////////////////////////////////////////////////////////////////////
    //! The coupling context
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    //! TODO: this is the simplest context -> just the solutionvector
    ////////////////////////////////////////////////////////////////////////////
    SolutionVector curSol_;
};

} // end namespace Dumux

#endif
