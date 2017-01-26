

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
 * \brief Test for the staggered grid (Navier-)Stokes model with analytical solution (Donea et al., 2003)
 */
#ifndef DUMUX_DONEA_TEST_PROBLEM_HH
#define DUMUX_DONEA_TEST_PROBLEM_HH

#include <dumux/implicit/staggered/properties.hh>
#include <dumux/freeflow/staggered/model.hh>
#include <dumux/implicit/problem.hh>
#include <dumux/material/components/simpleh2o.hh>
#include <dumux/material/fluidsystems/liquidphase.hh>
#include <dumux/material/components/constant.hh>


#include <dumux/linear/amgbackend.hh>


namespace Dumux
{
template <class TypeTag>
class DoneaTestProblem;

namespace Capabilities
{
    template<class TypeTag>
    struct isStationary<DoneaTestProblem<TypeTag>>
    { static const bool value = true; };
}

namespace Properties
{
NEW_TYPE_TAG(DoneaTestProblem, INHERITS_FROM(StaggeredModel, NavierStokes));

SET_PROP(DoneaTestProblem, Fluid)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
public:
    typedef FluidSystems::LiquidPhase<Scalar, Dumux::Constant<TypeTag, Scalar> > type;
};

// Set the grid type
SET_TYPE_PROP(DoneaTestProblem, Grid, Dune::YaspGrid<2>);

// Set the problem property
SET_TYPE_PROP(DoneaTestProblem, Problem, Dumux::DoneaTestProblem<TypeTag> );

SET_BOOL_PROP(DoneaTestProblem, EnableGlobalFVGeometryCache, true);

SET_BOOL_PROP(DoneaTestProblem, EnableGlobalFluxVariablesCache, true);
SET_BOOL_PROP(DoneaTestProblem, EnableGlobalVolumeVariablesCache, true);

// Enable gravity
SET_BOOL_PROP(DoneaTestProblem, ProblemEnableGravity, true);

#if ENABLE_NAVIERSTOKES
SET_BOOL_PROP(DoneaTestProblem, EnableInertiaTerms, true);
#else
SET_BOOL_PROP(DoneaTestProblem, EnableInertiaTerms, false);
#endif
}

/*!
 * \ingroup ImplicitTestProblems
 * \brief  Test problem for the staggered grid (Donea et al., 2003)
 */
template <class TypeTag>
class DoneaTestProblem : public NavierStokesProblem<TypeTag>
{
    typedef NavierStokesProblem<TypeTag> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;

    // copy some indices for convenience
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {
        // Grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };
    enum {
        massBalanceIdx = Indices::massBalanceIdx,
        momentumBalanceIdx = Indices::momentumBalanceIdx,
        momentumXBalanceIdx = Indices::momentumXBalanceIdx,
        momentumYBalanceIdx = Indices::momentumYBalanceIdx,
        pressureIdx = Indices::pressureIdx,
        velocityXIdx = Indices::velocityXIdx,
        velocityYIdx = Indices::velocityYIdx
    };

    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::Intersection Intersection;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, SubControlVolume) SubControlVolume;

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    using CellCenterPrimaryVariables = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using FacePrimaryVariables = typename GET_PROP_TYPE(TypeTag, FacePrimaryVariables);

    using BoundaryValues = typename GET_PROP_TYPE(TypeTag, BoundaryValues);
    using InitialValues = typename GET_PROP_TYPE(TypeTag, BoundaryValues);
    using SourceValues = typename GET_PROP_TYPE(TypeTag, BoundaryValues);

    using DofTypeIndices = typename GET_PROP(TypeTag, DofTypeIndices);
    typename DofTypeIndices::CellCenterIdx cellCenterIdx;
    typename DofTypeIndices::FaceIdx faceIdx;

public:
    DoneaTestProblem(TimeManager &timeManager, const GridView &gridView)
    : ParentType(timeManager, gridView), eps_(1e-6)
    {
        name_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag,
                                             std::string,
                                             Problem,
                                             Name);

        printL2Error_ = GET_RUNTIME_PARAM_FROM_GROUP(TypeTag,
                                                     bool,
                                                     Problem,
                                                     PrintL2Error);
    }

    /*!
     * \name Problem parameters
     */
    // \{

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     */
    std::string name() const
    {
        return name_;
    }

    bool shouldWriteRestartFile() const
    {
        return false;
    }

    void postTimeStep() const
    {
        if(printL2Error_)
        {
            const auto l2error = calculateL2Error();
            const int numCellCenterDofs = this->model().numCellCenterDofs();
            const int numFaceDofs = this->model().numFaceDofs();
            std::cout << std::setprecision(8) << "** L2 error (abs/rel) for "
                    << std::setw(6) << numCellCenterDofs << " cc dofs and " << numFaceDofs << " face dofs (total: " << numCellCenterDofs + numFaceDofs << "): "
                    << std::scientific
                    << "L2(p) = " << l2error.first[pressureIdx] << " / " << l2error.second[pressureIdx]
                    << ", L2(vx) = " << l2error.first[velocityXIdx] << " / " << l2error.second[velocityXIdx]
                    << ", L2(vy) = " << l2error.first[velocityYIdx] << " / " << l2error.second[velocityYIdx]
                    << std::endl;
        }
    }

    /*!
     * \brief Return the temperature within the domain in [K].
     *
     * This problem assumes a temperature of 10 degrees Celsius.
     */
    Scalar temperature() const
    { return 298.0; }

    /*!
     * \brief Return the sources within the domain.
     *
     * \param values Stores the source values, acts as return value
     * \param globalPos The global position
     */
    SourceValues sourceAtPos(const GlobalPosition &globalPos) const
    {
        SourceValues source(0.0);
        Scalar x = globalPos[0];
        Scalar y = globalPos[1];

        source[momentumXBalanceIdx] = (12.0-24.0*y) * x*x*x*x + (-24.0 + 48.0*y)* x*x*x
                                      + (-48.0*y + 72.0*y*y - 48.0*y*y*y + 12.0)* x*x
                                      + (-2.0 + 24.0*y - 72.0*y*y + 48.0*y*y*y)*x
                                      + 1.0 - 4.0*y + 12.0*y*y - 8.0*y*y*y;
        source[momentumYBalanceIdx] = (8.0 - 48.0*y + 48.0*y*y)*x*x*x + (-12.0 + 72.0*y - 72.0*y*y)*x*x
                                      + (4.0 - 24.0*y + 48.0*y*y - 48.0*y*y*y + 24.0*y*y*y*y)*x - 12.0*y*y
                                      + 24.0*y*y*y - 12.0*y*y*y*y;
        return source;
    }
    // \}
    /*!
     * \name Boundary conditions
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary control volume.
     *
     * \param values The boundary types for the conservation equations
     * \param globalPos The position of the center of the finite volume
     */
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition &globalPos) const
    {
        BoundaryTypes values;

        // set Dirichlet values for the velocity and pressure everywhere
        values.setDirichlet(momentumBalanceIdx);
        values.setDirichlet(massBalanceIdx);

        return values;
    }


    using ParentType::dirichletAtPos;
    BoundaryValues dirichletAtPos(const GlobalPosition & globalPos) const
    {
        Scalar x = globalPos[0];
        Scalar y = globalPos[1];

        BoundaryValues values;
        values[pressureIdx] = x * (1.0-x); // p(x,y) = x(1-x) [Donea2003]
        values[velocityXIdx] = x*x * (1.0 - x)*(1.0 - x) * (2.0*y - 6.0*y*y + 4.0*y*y*y);
        values[velocityYIdx] = -1.0*y*y * (1.0 - y)*(1.0 - y) * (2.0*x - 6.0*x*x + 4.0*x*x*x);

        return values;
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * For this method, the \a priVars parameter stores the mass flux
     * in normal direction of each component. Negative values mean
     * influx.
     */
    CellCenterPrimaryVariables neumannAtPos(const GlobalPosition& globalPos) const
    {
        return CellCenterPrimaryVariables(0);
    }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * For this method, the \a priVars parameter stores primary
     * variables.
     */
    using ParentType::initial;
    InitialValues initialAtPos(const GlobalPosition &globalPos) const
    {
        InitialValues values;
        values[pressureIdx] = 0.0;
        values[velocityXIdx] = 0.0;
        values[velocityYIdx] = 0.0;

        return values;
    }

    /*!
     * \brief Append all quantities of interest which can be derived
     *        from the solution of the current time step to the VTK
     *        writer.
     */
    void addOutputVtkFields()
    {
        //Here we calculate the analytical solution
        const auto numElements = this->gridView().size(0);
        auto& pExact = *(this->resultWriter().allocateManagedBuffer(numElements));
        auto& velocityExact = *(this->resultWriter()).template allocateManagedBuffer<double, dimWorld>(numElements);

        using DofTypeIndices = typename GET_PROP(TypeTag, DofTypeIndices);
        typename DofTypeIndices::FaceIdx faceIdx;

        const auto someElement = *(elements(this->gridView()).begin());

        auto someFvGeometry = localView(this->model().globalFvGeometry());
        someFvGeometry.bindElement(someElement);
        const auto someScv = *(scvs(someFvGeometry).begin());

        Scalar time = std::max(this->timeManager().time() + this->timeManager().timeStepSize(), 1e-10);

        for (const auto& element : elements(this->gridView()))
        {
            auto fvGeometry = localView(this->model().globalFvGeometry());
            fvGeometry.bindElement(element);

            for (auto&& scv : scvs(fvGeometry))
            {
                auto dofIdxGlobal = scv.dofIndex();
                const auto& globalPos = scv.dofPosition();

                pExact[dofIdxGlobal] = dirichletAtPos(globalPos)[pressureIdx];

                GlobalPosition velocityVector(0.0);
                for (auto&& scvf : scvfs(fvGeometry))
                {
                    auto dirIdx = scvf.directionIndex();
                    velocityVector[dirIdx] += 0.5*dirichletAtPos(globalPos)[faceIdx][dirIdx];
                }
                velocityExact[dofIdxGlobal] = velocityVector;
            }
        }
        this->resultWriter().attachDofData(pExact, "p_exact", false);
        this->resultWriter().attachDofData(velocityExact,  "velocity_exact", false, dim);
    }

    /*!
     * \brief Calculate the L2 error between the analytical solution and the numerical approximation.
     *
     */
    auto calculateL2Error() const
    {
        BoundaryValues sumError(0.0), sumReference(0.0), l2NormAbs(0.0), l2NormRel(0.0);

        const int numFaceDofs = this->model().numFaceDofs();

        std::vector<Scalar> staggeredVolume(numFaceDofs);
        std::vector<Scalar> errorVelocity(numFaceDofs);
        std::vector<Scalar> velocityReference(numFaceDofs);
        std::vector<int> directionIndex(numFaceDofs);

        Scalar totalVolume = 0.0;

        for (const auto& element : elements(this->gridView()))
        {
            auto fvGeometry = localView(this->model().globalFvGeometry());
            fvGeometry.bindElement(element);

            for (auto&& scv : scvs(fvGeometry))
            {
                // treat cell-center dofs
                const auto dofIdxCellCenter = scv.dofIndex();
                const auto& posCellCenter = scv.dofPosition();
                const auto analyticalSolutionCellCenter = dirichletAtPos(posCellCenter)[cellCenterIdx];
                const auto numericalSolutionCellCenter = this->model().curSol()[cellCenterIdx][dofIdxCellCenter];
                sumError[cellCenterIdx] += squaredDiff(analyticalSolutionCellCenter, numericalSolutionCellCenter) * scv.volume();
                sumReference[cellCenterIdx] += analyticalSolutionCellCenter * analyticalSolutionCellCenter * scv.volume();
                totalVolume += scv.volume();

                // treat face dofs
                for (auto&& scvf : scvfs(fvGeometry))
                {
                    const int dofIdxFace = scvf.dofIndexSelf();
                    const int dirIdx = scvf.directionIndex();
                    const auto analyticalSolutionFace = dirichletAtPos(scvf.center())[faceIdx][dirIdx];
                    const auto numericalSolutionFace = this->model().curSol()[faceIdx][dofIdxFace][momentumBalanceIdx];
                    directionIndex[dofIdxFace] = dirIdx;
                    errorVelocity[dofIdxFace] = squaredDiff(analyticalSolutionFace, numericalSolutionFace);
                    velocityReference[dofIdxFace] = squaredDiff(analyticalSolutionFace, 0.0);
                    const Scalar staggeredHalfVolume = 0.5 * scv.volume();
                    staggeredVolume[dofIdxFace] = staggeredVolume[dofIdxFace] + staggeredHalfVolume;
                }
            }
        }

        // get the absolute and relative discrete L2-error for cell-center dofs
        l2NormAbs[cellCenterIdx] = std::sqrt(sumError[cellCenterIdx] / totalVolume);
        l2NormRel[cellCenterIdx] = std::sqrt(sumError[cellCenterIdx] / sumReference[cellCenterIdx]);

        // get the absolute and relative discrete L2-error for face dofs
        for(int i = 0; i < numFaceDofs; ++i)
        {
            const int dirIdx = directionIndex[i];
            const auto error = errorVelocity[i];
            const auto ref = velocityReference[i];
            const auto volume = staggeredVolume[i];
            sumError[faceIdx][dirIdx] += error * volume;
            sumReference[faceIdx][dirIdx] += ref * volume;
        }

        for(int dirIdx = 0; dirIdx < dimWorld; ++dirIdx)
        {
            l2NormAbs[faceIdx][dirIdx] = std::sqrt(sumError[faceIdx][dirIdx] / totalVolume);
            l2NormRel[faceIdx][dirIdx] = std::sqrt(sumError[faceIdx][dirIdx] / sumReference[faceIdx][dirIdx]);
        }
        return std::make_pair(l2NormAbs, l2NormRel);
    }


private:
    template<class T>
    T squaredDiff(const T& a, const T& b) const
    {
        return (a-b)*(a-b);
    }

    Scalar eps_;
    std::string name_;
    bool printL2Error_;

};
} //end namespace

#endif
