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
 * \ingroup StokesDarcyCoupling
 * \copydoc Dumux::StokesDarcyCouplingManager
 */

#ifndef DUMUX_STOKES_DARCY_COUPLINGMANAGER_TPFA_HH
#define DUMUX_STOKES_DARCY_COUPLINGMANAGER_TPFA_HH

#include <utility>
#include <memory>

#include <dune/common/float_cmp.hh>
#include <dune/common/exceptions.hh>
#include <dumux/common/properties.hh>
#include <dumux/multidomain/staggeredcouplingmanager.hh>
#include <dumux/discretization/staggered/elementsolution.hh>

#include "../../couplingmanager.hh"
#include "../../couplingdata.hh"
#include "couplingmapper.hh"

namespace Dumux {

/*!
 * \ingroup StokesDarcyCoupling
 * \brief Coupling manager for Stokes and Darcy domains with equal dimension.
 */
template<class MDTraits>
class StokesDarcyCouplingManagerImplementation<MDTraits, DiscretizationMethod::cctpfa>
: public virtual StaggeredCouplingManager<MDTraits>, public FreeFlowPorousMediumCouplingManagerBase<MDTraits>
{
    using Scalar = typename MDTraits::Scalar;
    using ParentType = StaggeredCouplingManager<MDTraits>;

public:
    static constexpr auto freeFlowFaceIdx = FreeFlowPorousMediumCouplingManagerBase<MDTraits>::freeFlowFaceIdx;
    static constexpr auto freeFlowCellCenterIdx = FreeFlowPorousMediumCouplingManagerBase<MDTraits>::freeFlowCellCenterIdx;
    static constexpr auto freeFlowIdx = FreeFlowPorousMediumCouplingManagerBase<MDTraits>::freeFlowIdx;
    static constexpr auto porousMediumIdx = FreeFlowPorousMediumCouplingManagerBase<MDTraits>::porousMediumIdx;

private:

    using SolutionVector = typename MDTraits::SolutionVector;

    // obtain the type tags of the sub problems
    using StokesTypeTag = typename MDTraits::template SubDomain<0>::TypeTag;
    using DarcyTypeTag = typename MDTraits::template SubDomain<2>::TypeTag;

    using CouplingStencils = std::unordered_map<std::size_t, std::vector<std::size_t> >;
    using CouplingStencil = CouplingStencils::mapped_type;

    // the sub domain type tags
    template<std::size_t id>
    using SubDomainTypeTag = typename MDTraits::template SubDomain<id>::TypeTag;

    static constexpr bool isCompositional = GetPropType<SubDomainTypeTag<0>, Properties::ModelTraits>::numFluidComponents()> 1;

    template<std::size_t id> using Problem = GetPropType<SubDomainTypeTag<id>, Properties::Problem>;
    template<std::size_t id> using NumEqVector = GetPropType<SubDomainTypeTag<id>, Properties::NumEqVector>;
    template<std::size_t id> using ElementVolumeVariables = typename GetPropType<SubDomainTypeTag<id>, Properties::GridVolumeVariables>::LocalView;
    template<std::size_t id> using GridVolumeVariables = GetPropType<SubDomainTypeTag<id>, Properties::GridVolumeVariables>;
    template<std::size_t id> using VolumeVariables = typename GetPropType<SubDomainTypeTag<id>, Properties::GridVolumeVariables>::VolumeVariables;
    template<std::size_t id> using GridGeometry = GetPropType<SubDomainTypeTag<id>, Properties::GridGeometry>;
    template<std::size_t id> using FVElementGeometry = typename GridGeometry<id>::LocalView;
    template<std::size_t id> using ElementBoundaryTypes = GetPropType<SubDomainTypeTag<id>, Properties::ElementBoundaryTypes>;
    template<std::size_t id> using ElementFluxVariablesCache = typename GetPropType<SubDomainTypeTag<id>, Properties::GridFluxVariablesCache>::LocalView;
    template<std::size_t id> using GridVariables = GetPropType<SubDomainTypeTag<id>, Properties::GridVariables>;
    template<std::size_t id> using GridView = typename GridGeometry<id>::GridView;
    template<std::size_t id> using Element = typename GridView<id>::template Codim<0>::Entity;
    template<std::size_t id> using PrimaryVariables = typename MDTraits::template SubDomain<id>::PrimaryVariables;
    template<std::size_t id> using SubControlVolumeFace  = typename FVElementGeometry<id>::SubControlVolumeFace;

    using CellCenterSolutionVector = GetPropType<StokesTypeTag, Properties::CellCenterSolutionVector>;

    using VelocityVector = typename Element<freeFlowIdx>::Geometry::GlobalCoordinate;

    using CouplingMapper = StokesDarcyCouplingMapperTpfa<MDTraits>;

    struct StationaryStokesCouplingContext
    {
        Element<porousMediumIdx> element;
        FVElementGeometry<porousMediumIdx> fvGeometry;
        std::size_t darcyScvfIdx;
        std::size_t stokesScvfIdx;
        VolumeVariables<porousMediumIdx> volVars;

        auto permeability() const
        {
            return volVars.permeability();
        }
    };

    struct StationaryDarcyCouplingContext
    {
        Element<freeFlowIdx> element;
        FVElementGeometry<freeFlowIdx> fvGeometry;
        std::size_t stokesScvfIdx;
        std::size_t darcyScvfIdx;
        VelocityVector velocity;
        VolumeVariables<freeFlowIdx> volVars;
    };
public:

    using ParentType::couplingStencil;
    using ParentType::updateCouplingContext;
    using CouplingData = StokesDarcyCouplingData<MDTraits, StokesDarcyCouplingManager<MDTraits>>;

    //! Constructor
    StokesDarcyCouplingManagerImplementation(std::shared_ptr<const GridGeometry<freeFlowIdx>> stokesFvGridGeometry,
                               std::shared_ptr<const GridGeometry<porousMediumIdx>> darcyFvGridGeometry)
    { }

    /*!
     * \brief Methods to be accessed by main
     */
    // \{

    //! Initialize the coupling manager
    void init(std::shared_ptr<const Problem<freeFlowIdx>> stokesProblem,
              std::shared_ptr<const Problem<porousMediumIdx>> darcyProblem,
              const SolutionVector& curSol)
    {
        if (Dune::FloatCmp::ne(stokesProblem->gravity(), darcyProblem->spatialParams().gravity({})))
            DUNE_THROW(Dune::InvalidStateException, "Both models must use the same gravity vector");

        this->setSubProblems(std::make_tuple(stokesProblem, stokesProblem, darcyProblem));
        this->curSol() = curSol;
        couplingData_ = std::make_shared<CouplingData>(*this);
        computeStencils();
    }

    //! Update after the grid has changed
    void update()
    { }

    // \}

    //! Update the solution vector before assembly
    void updateSolution(const SolutionVector& curSol)
    { this->curSol() = curSol; }

    //! Prepare the coupling stencils
    void computeStencils()
    {
        couplingMapper_.computeCouplingMapsAndStencils(*this,
                                                       darcyToStokesCellCenterCouplingStencils_,
                                                       darcyToStokesFaceCouplingStencils_,
                                                       stokesCellCenterCouplingStencils_,
                                                       stokesFaceCouplingStencils_);

        for(auto&& stencil : darcyToStokesCellCenterCouplingStencils_)
            removeDuplicates_(stencil.second);
        for(auto&& stencil : darcyToStokesFaceCouplingStencils_)
            removeDuplicates_(stencil.second);
        for(auto&& stencil : stokesCellCenterCouplingStencils_)
            removeDuplicates_(stencil.second);
        for(auto&& stencil : stokesFaceCouplingStencils_)
            removeDuplicates_(stencil.second);
    }

    /*!
     * \brief Methods to be accessed by the assembly
     */
    // \{

    using ParentType::evalCouplingResidual;

    /*!
     * \brief prepares all data and variables that are necessary to evaluate the residual of an Darcy element (i.e. Darcy information)
     */
    template<std::size_t i, class Assembler, std::enable_if_t<(i == freeFlowCellCenterIdx || i == freeFlowFaceIdx), int> = 0>
    void bindCouplingContext(Dune::index_constant<i> domainI, const Element<freeFlowCellCenterIdx>& element, const Assembler& assembler) const
    { bindCouplingContext(domainI, element); }

    /*!
     * \brief prepares all data and variables that are necessary to evaluate the residual of an Darcy element (i.e. Darcy information)
     */
    template<std::size_t i, std::enable_if_t<(i == freeFlowCellCenterIdx || i == freeFlowFaceIdx), int> = 0>
    void bindCouplingContext(Dune::index_constant<i> domainI, const Element<freeFlowCellCenterIdx>& element) const
    {
        stokesCouplingContext_.clear();

        const auto stokesElementIdx = this->problem(freeFlowIdx).gridGeometry().elementMapper().index(element);
        boundStokesElemIdx_ = stokesElementIdx;

        // do nothing if the element is not coupled to the other domain
        if(!couplingMapper_.stokesElementToDarcyElementMap().count(stokesElementIdx))
            return;

        // prepare the coupling context
        const auto& darcyIndices = couplingMapper_.stokesElementToDarcyElementMap().at(stokesElementIdx);
        auto darcyFvGeometry = localView(this->problem(porousMediumIdx).gridGeometry());

        for(auto&& indices : darcyIndices)
        {
            const auto& darcyElement = this->problem(porousMediumIdx).gridGeometry().boundingBoxTree().entitySet().entity(indices.eIdx);
            darcyFvGeometry.bindElement(darcyElement);
            const auto& scv = (*scvs(darcyFvGeometry).begin());

            const auto darcyElemSol = elementSolution(darcyElement, this->curSol()[porousMediumIdx], this->problem(porousMediumIdx).gridGeometry());
            VolumeVariables<porousMediumIdx> darcyVolVars;
            darcyVolVars.update(darcyElemSol, this->problem(porousMediumIdx), darcyElement, scv);

            // add the context
            stokesCouplingContext_.push_back({darcyElement, darcyFvGeometry, indices.scvfIdx, indices.flipScvfIdx, darcyVolVars});
        }
    }

    /*!
     * \brief prepares all data and variables that are necessary to evaluate the residual of an Darcy element (i.e. Stokes information)
     */
    template<class Assembler>
    void bindCouplingContext(Dune::index_constant<porousMediumIdx> domainI, const Element<porousMediumIdx>& element, const Assembler& assembler) const
    { bindCouplingContext(domainI, element); }

    /*!
     * \brief prepares all data and variables that are necessary to evaluate the residual of an Darcy element (i.e. Stokes information)
     */
    void bindCouplingContext(Dune::index_constant<porousMediumIdx> domainI, const Element<porousMediumIdx>& element) const
    {
        darcyCouplingContext_.clear();

        const auto darcyElementIdx = this->problem(porousMediumIdx).gridGeometry().elementMapper().index(element);
        boundDarcyElemIdx_ = darcyElementIdx;

        // do nothing if the element is not coupled to the other domain
        if(!couplingMapper_.darcyElementToStokesElementMap().count(darcyElementIdx))
            return;

        // prepare the coupling context
        const auto& stokesElementIndices = couplingMapper_.darcyElementToStokesElementMap().at(darcyElementIdx);
        auto stokesFvGeometry = localView(this->problem(freeFlowIdx).gridGeometry());

        for(auto&& indices : stokesElementIndices)
        {
            const auto& stokesElement = this->problem(freeFlowIdx).gridGeometry().boundingBoxTree().entitySet().entity(indices.eIdx);
            stokesFvGeometry.bindElement(stokesElement);

            VelocityVector faceVelocity(0.0);

            for(const auto& scvf : scvfs(stokesFvGeometry))
            {
                if(scvf.index() == indices.scvfIdx)
                    faceVelocity[scvf.directionIndex()] = this->curSol()[freeFlowFaceIdx][scvf.dofIndex()];
            }

            using PriVarsType = typename VolumeVariables<freeFlowCellCenterIdx>::PrimaryVariables;
            const auto& cellCenterPriVars = this->curSol()[freeFlowCellCenterIdx][indices.eIdx];
            const auto elemSol = makeElementSolutionFromCellCenterPrivars<PriVarsType>(cellCenterPriVars);

            VolumeVariables<freeFlowIdx> stokesVolVars;
            for(const auto& scv : scvs(stokesFvGeometry))
                stokesVolVars.update(elemSol, this->problem(freeFlowIdx), stokesElement, scv);

            // add the context
            darcyCouplingContext_.push_back({stokesElement, stokesFvGeometry, indices.scvfIdx, indices.flipScvfIdx, faceVelocity, stokesVolVars});
        }
    }

    /*!
     * \brief Update the coupling context for the Darcy residual w.r.t. Darcy DOFs
     */
    template<class LocalAssemblerI>
    void updateCouplingContext(Dune::index_constant<porousMediumIdx> domainI,
                               const LocalAssemblerI& localAssemblerI,
                               Dune::index_constant<porousMediumIdx> domainJ,
                               std::size_t dofIdxGlobalJ,
                               const PrimaryVariables<porousMediumIdx>& priVarsJ,
                               int pvIdxJ)
    {
        this->curSol()[domainJ][dofIdxGlobalJ][pvIdxJ] = priVarsJ[pvIdxJ];
    }

    /*!
     * \brief Update the coupling context for the Darcy residual w.r.t. the Stokes cell-center DOFs (DarcyToCC)
     */
    template<class LocalAssemblerI>
    void updateCouplingContext(Dune::index_constant<porousMediumIdx> domainI,
                               const LocalAssemblerI& localAssemblerI,
                               Dune::index_constant<freeFlowCellCenterIdx> domainJ,
                               const std::size_t dofIdxGlobalJ,
                               const PrimaryVariables<freeFlowCellCenterIdx>& priVars,
                               int pvIdxJ)
    {
        this->curSol()[domainJ][dofIdxGlobalJ] = priVars;

        for (auto& data : darcyCouplingContext_)
        {
            const auto stokesElemIdx = this->problem(freeFlowIdx).gridGeometry().elementMapper().index(data.element);

            if(stokesElemIdx != dofIdxGlobalJ)
                continue;

            using PriVarsType = typename VolumeVariables<freeFlowCellCenterIdx>::PrimaryVariables;
            const auto elemSol = makeElementSolutionFromCellCenterPrivars<PriVarsType>(priVars);

            for(const auto& scv : scvs(data.fvGeometry))
                data.volVars.update(elemSol, this->problem(freeFlowIdx), data.element, scv);
        }
    }

    /*!
     * \brief Update the coupling context for the Darcy residual w.r.t. the Stokes face DOFs (DarcyToFace)
     */
    template<class LocalAssemblerI>
    void updateCouplingContext(Dune::index_constant<porousMediumIdx> domainI,
                               const LocalAssemblerI& localAssemblerI,
                               Dune::index_constant<freeFlowFaceIdx> domainJ,
                               const std::size_t dofIdxGlobalJ,
                               const PrimaryVariables<freeFlowFaceIdx>& priVars,
                               int pvIdxJ)
    {
        this->curSol()[domainJ][dofIdxGlobalJ] = priVars;

        for (auto& data : darcyCouplingContext_)
        {
            for(const auto& scvf : scvfs(data.fvGeometry))
            {
                if(scvf.dofIndex() == dofIdxGlobalJ)
                    data.velocity[scvf.directionIndex()] = priVars;
            }
        }
    }

    /*!
     * \brief Update the coupling context for the Stokes cc residual w.r.t. the Darcy DOFs (FaceToDarcy)
     */
    template<std::size_t i, class LocalAssemblerI, std::enable_if_t<(i == freeFlowCellCenterIdx || i == freeFlowFaceIdx), int> = 0>
    void updateCouplingContext(Dune::index_constant<i> domainI,
                               const LocalAssemblerI& localAssemblerI,
                               Dune::index_constant<porousMediumIdx> domainJ,
                               const std::size_t dofIdxGlobalJ,
                               const PrimaryVariables<porousMediumIdx>& priVars,
                               int pvIdxJ)
    {
        this->curSol()[domainJ][dofIdxGlobalJ] = priVars;

        for (auto& data : stokesCouplingContext_)
        {
            const auto darcyElemIdx = this->problem(porousMediumIdx).gridGeometry().elementMapper().index(data.element);

            if(darcyElemIdx != dofIdxGlobalJ)
                continue;

            const auto darcyElemSol = elementSolution(data.element, this->curSol()[porousMediumIdx], this->problem(porousMediumIdx).gridGeometry());

            for(const auto& scv : scvs(data.fvGeometry))
                data.volVars.update(darcyElemSol, this->problem(porousMediumIdx), data.element, scv);
        }
    }

    // \}

    /*!
     * \brief Access the coupling data
     */
    const auto& couplingData() const
    {
        return *couplingData_;
    }

    /*!
     * \brief Access the coupling context needed for the Stokes domain
     */
    const auto& stokesCouplingContext(const Element<freeFlowIdx>& element, const SubControlVolumeFace<freeFlowIdx>& scvf) const
    {
        if (stokesCouplingContext_.empty() || boundStokesElemIdx_ != scvf.insideScvIdx())
            bindCouplingContext(freeFlowIdx, element);

        for(const auto& context : stokesCouplingContext_)
        {
            if(scvf.index() == context.stokesScvfIdx)
                return context;
        }

        DUNE_THROW(Dune::InvalidStateException, "No coupling context found at scvf " << scvf.center());
    }

    /*!
     * \brief Access the coupling context needed for the Darcy domain
     */
    const auto& darcyCouplingContext(const Element<porousMediumIdx>& element, const SubControlVolumeFace<porousMediumIdx>& scvf) const
    {
        if (darcyCouplingContext_.empty() || boundDarcyElemIdx_ != scvf.insideScvIdx())
            bindCouplingContext(porousMediumIdx, element);

        for(const auto& context : darcyCouplingContext_)
        {
            if(scvf.index() == context.darcyScvfIdx)
                return context;
        }

        DUNE_THROW(Dune::InvalidStateException, "No coupling context found at scvf " << scvf.center());
    }

    /*!
     * \brief The coupling stencils
     */
    // \{

    /*!
     * \brief The Stokes cell center coupling stencil w.r.t. Darcy DOFs
     */
    const CouplingStencil& couplingStencil(Dune::index_constant<freeFlowCellCenterIdx> domainI,
                                           const Element<freeFlowIdx>& element,
                                           Dune::index_constant<porousMediumIdx> domainJ) const
    {
        const auto eIdx = this->problem(domainI).gridGeometry().elementMapper().index(element);
        if(stokesCellCenterCouplingStencils_.count(eIdx))
            return stokesCellCenterCouplingStencils_.at(eIdx);
        else
            return emptyStencil_;
    }

    /*!
     * \brief The coupling stencil of domain I, i.e. which domain J DOFs
     *        the given domain I element's residual depends on.
     */
    template<std::size_t i, std::size_t j>
    const CouplingStencil& couplingStencil(Dune::index_constant<i> domainI,
                                           const Element<i>& element,
                                           Dune::index_constant<j> domainJ) const
    { return emptyStencil_; }

    /*!
     * \brief The coupling stencil of domain I, i.e. which domain J dofs
     *        the given domain I element's residual depends on.
     */
    const CouplingStencil& couplingStencil(Dune::index_constant<porousMediumIdx> domainI,
                                           const Element<porousMediumIdx>& element,
                                           Dune::index_constant<freeFlowCellCenterIdx> domainJ) const
    {
        const auto eIdx = this->problem(domainI).gridGeometry().elementMapper().index(element);
        if(darcyToStokesCellCenterCouplingStencils_.count(eIdx))
            return darcyToStokesCellCenterCouplingStencils_.at(eIdx);
        else
            return emptyStencil_;
    }

    /*!
     * \brief The coupling stencil of domain I, i.e. which domain J dofs
     *        the given domain I element's residual depends on.
     */
    const CouplingStencil& couplingStencil(Dune::index_constant<porousMediumIdx> domainI,
                                           const Element<porousMediumIdx>& element,
                                           Dune::index_constant<freeFlowFaceIdx> domainJ) const
    {
        const auto eIdx = this->problem(domainI).gridGeometry().elementMapper().index(element);
        if (darcyToStokesFaceCouplingStencils_.count(eIdx))
            return darcyToStokesFaceCouplingStencils_.at(eIdx);
        else
            return emptyStencil_;
    }

    /*!
     * \brief The coupling stencil of domain I, i.e. which domain J DOFs
     *        the given domain I element's residual depends on.
     */
    template<std::size_t i, std::size_t j>
    const CouplingStencil& couplingStencil(Dune::index_constant<i> domainI,
                                           const SubControlVolumeFace<freeFlowIdx>& scvf,
                                           Dune::index_constant<j> domainJ) const
    { return emptyStencil_; }

    /*!
     * \brief The coupling stencil of a Stokes face w.r.t. Darcy DOFs
     */
    const CouplingStencil& couplingStencil(Dune::index_constant<freeFlowFaceIdx> domainI,
                                           const SubControlVolumeFace<freeFlowIdx>& scvf,
                                           Dune::index_constant<porousMediumIdx> domainJ) const
    {
        const auto faceDofIdx = scvf.dofIndex();
        if(stokesFaceCouplingStencils_.count(faceDofIdx))
            return stokesFaceCouplingStencils_.at(faceDofIdx);
        else
            return emptyStencil_;
    }

    // \}

    /*!
     * \brief There are no additional dof dependencies
     */
    template<class IdType>
    const std::vector<std::size_t>& getAdditionalDofDependencies(IdType id, std::size_t stokesElementIdx) const
    { return emptyStencil_; }

    /*!
     * \brief There are no additional dof dependencies
     */
    template<class IdType>
    const std::vector<std::size_t>& getAdditionalDofDependenciesInverse(IdType id, std::size_t darcyElementIdx) const
    { return emptyStencil_; }

    /*!
     * \brief Returns whether a given free flow scvf is coupled to the other domain
     */
    bool isCoupledEntity(Dune::index_constant<freeFlowIdx>, const SubControlVolumeFace<freeFlowFaceIdx>& scvf) const
    {
        return stokesFaceCouplingStencils_.count(scvf.dofIndex());
    }

    /*!
     * \brief Returns whether a given free flow scvf is coupled to the other domain
     */
    bool isCoupledEntity(Dune::index_constant<porousMediumIdx>, const SubControlVolumeFace<porousMediumIdx>& scvf) const
    {
        return couplingMapper_.isCoupledDarcyScvf(scvf.index());
    }

    /*!
     * \brief Returns whether a given free flow scvf is coupled to the other domain
     */
    bool isCoupledEntity(Dune::index_constant<porousMediumIdx>, const Element<porousMediumIdx>& element, const SubControlVolumeFace<porousMediumIdx>& scvf) const
    {
        return couplingMapper_.isCoupledDarcyScvf(scvf.index());
    }

protected:

    //! Return a reference to an empty stencil
    std::vector<std::size_t>& emptyStencil()
    { return emptyStencil_; }

    void removeDuplicates_(std::vector<std::size_t>& stencil)
    {
        std::sort(stencil.begin(), stencil.end());
        stencil.erase(std::unique(stencil.begin(), stencil.end()), stencil.end());
    }

private:

    std::vector<bool> isCoupledDarcyDof_;
    std::shared_ptr<CouplingData> couplingData_;

    std::unordered_map<std::size_t, std::vector<std::size_t> > stokesCellCenterCouplingStencils_;
    std::unordered_map<std::size_t, std::vector<std::size_t> > stokesFaceCouplingStencils_;
    std::unordered_map<std::size_t, std::vector<std::size_t> > darcyToStokesCellCenterCouplingStencils_;
    std::unordered_map<std::size_t, std::vector<std::size_t> > darcyToStokesFaceCouplingStencils_;
    std::vector<std::size_t> emptyStencil_;

    ////////////////////////////////////////////////////////////////////////////
    //! The coupling context
    ////////////////////////////////////////////////////////////////////////////
    mutable std::vector<StationaryStokesCouplingContext> stokesCouplingContext_;
    mutable std::vector<StationaryDarcyCouplingContext> darcyCouplingContext_;

    mutable std::size_t boundStokesElemIdx_;
    mutable std::size_t boundDarcyElemIdx_;

    CouplingMapper couplingMapper_;
};

} // end namespace Dumux

#endif
