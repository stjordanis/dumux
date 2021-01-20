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
 * \ingroup RichardsTests
 * \brief Richards benchmarks base problem
 *
 * Infiltration benchmark:
 * Root-soil benchmark paper Schnepf et al. (case M2.1, Eq. 4) https://doi.org/10.3389/fpls.2020.00316
 * based on Vanderborght 2005 (see Fig. 4abc and Eq. 56-60) https://doi.org/10.2113/4.1.206
 *
 * Evaporation benchmark:
 * Root-soil benchmark paper Schnepf et al. (case M2.2) https://doi.org/10.3389/fpls.2020.00316
 * based on Vanderborght 2005 (see Fig. 5abcd and Eq. 39-47) https://doi.org/10.2113/4.1.206
 */

#ifndef DUMUX_RICHARDS_BECHMARK_PROBLEM_HH
#define DUMUX_RICHARDS_BECHMARK_PROBLEM_HH

#include <dumux/common/properties.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/boundarytypes.hh>

#include <dumux/porousmediumflow/problem.hh>
#include <dumux/material/components/simpleh2o.hh>

namespace Dumux {

enum class BenchmarkScenario {
    evaporation, infiltration
};

/*!
 * \ingroup RichardsTests
 * \brief Richards benchmarks base problem
 */
template <class TypeTag>
class RichardsBenchmarkProblem : public PorousMediumFlowProblem<TypeTag>
{
    using ParentType = PorousMediumFlowProblem<TypeTag>;

    using Scalar = GetPropType<TypeTag, Properties::Scalar>;
    using PrimaryVariables = GetPropType<TypeTag, Properties::PrimaryVariables>;
    using BoundaryTypes = Dumux::BoundaryTypes<GetPropType<TypeTag, Properties::ModelTraits>::numEq()>;
    using NumEqVector = GetPropType<TypeTag, Properties::NumEqVector>;

    using Indices = typename GetPropType<TypeTag, Properties::ModelTraits>::Indices;

    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    using FVElementGeometry = typename GridGeometry::LocalView;
    using SubControlVolumeFace = typename GridGeometry::SubControlVolumeFace;
    using GridView = typename GridGeometry::GridView;
    using Element = typename GridView::template Codim<0>::Entity;
    using GlobalPosition = typename GridView::template Codim<0>::Geometry::GlobalCoordinate;

    static constexpr int dimWorld = GridView::dimensionworld;

public:
    RichardsBenchmarkProblem(std::shared_ptr<const GridGeometry> gridGeometry)
    : ParentType(gridGeometry)
    {
        name_ = getParam<std::string>("Problem.Name");
        const auto density = Components::SimpleH2O<double>::liquidDensity(0,0);
        const auto initialHead = getParam<Scalar>("Problem.InitialHeadInCm")*0.01;
        const auto criticalHead = getParam<Scalar>("Problem.CriticalSurfaceHeadInCm")*0.01;
        initialPressure_ = 1.0e5 + initialHead*9.81*density;
        criticalSurfacePressure_ = 1.0e5 + criticalHead*9.81*density;
        const auto& fm = this->spatialParams().fluidMatrixInteractionAtPos(0.0);
        const auto criticalSaturation = fm.sw(1.0e5 - criticalSurfacePressure_);
        criticalSurfaceKrw_ = fm.krw(criticalSaturation);
        enableGravity_ = getParam<bool>("Problem.EnableGravity", true);

        const auto potentialRate = getParam<Scalar>("Problem.SurfaceFluxMilliMeterPerDay");
        potentialRate_ = density*potentialRate/(1000*86400.0); // mm/day -> kg/s/m^2
        useKrwAverage_ = getParam<bool>("Problem.UseKrwAverage", false);
        bottomDirichlet_ = getParam<bool>("Problem.BottomDirichlet", false);
        scenario_ = (potentialRate > 0) ? BenchmarkScenario::evaporation : BenchmarkScenario::infiltration;
        surfaceArea_ = (scenario_ == BenchmarkScenario::evaporation) ? 0.1*0.1 : 0.05*0.05;
    }

    // output name
    const std::string& name() const
    { return name_; }

    // reference temperature (unused but required)
    Scalar temperature() const
    { return 273.15 + 10; };

    // reference pressure
    Scalar nonwettingReferencePressure() const
    { return 1.0e5; };

    // column cross-section area
    Scalar extrusionFactorAtPos(const GlobalPosition &globalPos) const
    { return surfaceArea_; }

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     */
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition &globalPos) const
    {
        BoundaryTypes bcTypes;
        if (onLowerBoundary(globalPos) && !bottomDirichlet_)
            bcTypes.setAllNeumann();
        else if (onLowerBoundary(globalPos) && bottomDirichlet_)
            bcTypes.setAllDirichlet();
        else if (onUpperBoundary(globalPos))
            bcTypes.setAllNeumann();
        else
            DUNE_THROW(Dune::InvalidStateException, "Wrong boundary?");
        return bcTypes;
    }

    /*!
     * \brief Evaluates the boundary conditions for a Dirichlet boundary segment.
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition &globalPos) const
    { return initialAtPos(globalPos); }

    /*!
     * \brief Evaluates the initial values for a control volume
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos) const
    {
        PrimaryVariables values(0.0);
        values[Indices::pressureIdx] = initialPressure_;
        values.setState(Indices::bothPhases);
        return values;
    }

    /*!
     * \brief Evaluates the boundary conditions for a Neumann boundary segment.
     * Negative values mean influx.
     */
    template<class ElementVolumeVariables, class ElementFluxVariablesCache>
    NumEqVector neumann(const Element& element,
                        const FVElementGeometry& fvGeometry,
                        const ElementVolumeVariables& elemVolVars,
                        const ElementFluxVariablesCache& elemFluxVarsCache,
                        const SubControlVolumeFace& scvf) const
    {
        NumEqVector values(0.0);
        const auto& globalPos = scvf.ipGlobal();
        if (onUpperBoundary(globalPos))
        {
            const auto& volVars = elemVolVars[scvf.insideScvIdx()];

            const auto dist = (fvGeometry.scv(scvf.insideScvIdx()).center() - globalPos).two_norm();
            const auto cellPressure = volVars.pressure(0);
            const auto density = volVars.density(0);
            const auto viscosity = volVars.viscosity(0);
            const auto relPerm = volVars.relativePermeability(0);
            const auto K = volVars.permeability();
            const auto gravity = enableGravity_ ? 9.81 : 0.0;
            const auto avgRelPerm = 0.5*(relPerm + criticalSurfaceKrw_);

            // kg/m^3 * m^2 * Pa / m / Pa / s = kg/s/m^2
            auto criticalRate = density*K/viscosity*((cellPressure - criticalSurfacePressure_)/dist - density*gravity);
            if (!std::signbit(criticalRate))
                criticalRate *= useKrwAverage_ ? avgRelPerm : relPerm;

            if (scenario_ == BenchmarkScenario::evaporation)
                values[Indices::conti0EqIdx] = std::min(potentialRate_, criticalRate);
            else
                values[Indices::conti0EqIdx] = std::max(potentialRate_, criticalRate);
        }

        // free drainage (gravitational flux) for infiltration scenario
        else if (onLowerBoundary(globalPos) && (scenario_ == BenchmarkScenario::infiltration))
        {
            const auto& volVars = elemVolVars[scvf.insideScvIdx()];
            const auto gravity = enableGravity_ ? 9.81 : 0.0;
            const auto density = volVars.density(0);
            const auto viscosity = volVars.viscosity(0);
            const auto relPerm = volVars.relativePermeability(0);
            const auto K = volVars.permeability();

            values[Indices::conti0EqIdx] = density*K*relPerm/viscosity*(density*gravity);
        }

        return values;
    }

    bool onLowerBoundary(const GlobalPosition &globalPos) const
    { return globalPos[dimWorld-1] < this->gridGeometry().bBoxMin()[dimWorld-1] + eps_; }

    bool onUpperBoundary(const GlobalPosition &globalPos) const
    { return globalPos[dimWorld-1] > this->gridGeometry().bBoxMax()[dimWorld-1] - eps_; }

    //! compute the actual evaporation/infiltration rate
    template<class SolutionVector, class GridVariables>
    Scalar computeActualRate(const SolutionVector& sol, const GridVariables& gridVars, bool verbose = true) const
    {
        Scalar rate = 0.0;

        auto fvGeometry = localView(this->gridGeometry());
        auto elemVolVars = localView(gridVars.curGridVolVars());
        for (const auto& element : elements(this->gridGeometry().gridView()))
        {
            fvGeometry.bindElement(element);
            elemVolVars.bindElement(element, fvGeometry, sol);
            for (const auto& scvf : scvfs(fvGeometry))
                if (scvf.boundary())
                    rate += this->neumann(element, fvGeometry, elemVolVars, 0.0, scvf)[0];
        }

        if (verbose)
            std::cout << Fmt::format("Actual rate: {:.5g} (mm/day)\n", rate*86400*1000/1000);

        return rate*86400*1000/1000;
    }

private:
    Scalar initialPressure_, criticalSurfacePressure_, potentialRate_;
    Scalar criticalSurfaceKrw_;
    static constexpr Scalar eps_ = 1.5e-7;
    std::string name_;
    bool enableGravity_;
    bool bottomDirichlet_;
    bool useKrwAverage_;
    BenchmarkScenario scenario_;
    Scalar surfaceArea_;
};

} // end namespace Dumux

#endif