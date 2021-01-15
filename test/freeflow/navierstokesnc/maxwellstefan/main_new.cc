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
 * \ingroup NavierStokesNCTests
 * \brief Test for the staggered grid multi-component (Navier-)Stokes model.
 */

 #include <config.h>

 #include <ctime>
 #include <iostream>

 #include <dune/common/parallel/mpihelper.hh>
 #include <dune/common/timer.hh>
 #include <dune/istl/io.hh>

 #include <dumux/assembly/diffmethod.hh>
 #include <dumux/common/dumuxmessage.hh>
 #include <dumux/common/parameters.hh>
 #include <dumux/common/properties.hh>

 #include <dumux/io/grid/gridmanager.hh>
 #include <dumux/linear/seqsolverbackend.hh>

 #include <dumux/multidomain/fvassembler.hh>
 #include <dumux/multidomain/traits.hh>
 #include <dumux/multidomain/newtonsolver.hh>
 #include <dumux/multidomain/staggeredfreeflow/couplingmanager.hh>

 #include <dumux/io/vtkoutputmodule.hh>
 #include <dumux/io/gnuplotinterface.hh>
 #include <dumux/freeflow/navierstokes/velocityoutput.hh>

 #include "problem_new.hh"

 namespace Dumux::Properties {

 // Set the problem property
 template<class TypeTag>
 struct CouplingManager<TypeTag, TTag::MaxwellStefanNCTest>
 {
 private:
     using Traits = MultiDomainTraits<TTag::MaxwellStefanNCTestMomentum, TTag::MaxwellStefanNCTestMass>;
 public:
     using type = StaggeredFreeFlowCouplingManager<Traits>;
 };

}

namespace Dumux {

template<class Scalar>
class PlotConcentration
{
public:
    /*!
      * \brief Writes out the diffusion rates from left to right
      *
      * Called after every time step
      *
      * \param curSol Vector containing the current solution
      * \param gridVariables The grid variables
      * \param time The time
      */
      template<class SolutionVector, class GridVariables>
      void plotComponentsOverTime(const SolutionVector& curSol,
                                  const GridVariables& gridVariables,
                                  const Scalar time)
     {
         using FluidSystem = typename GridVariables::VolumeVariables::FluidSystem;
         const auto& gridGeometry = gridVariables.curGridVolVars().problem().gridGeometry();
         Scalar x_co2_left = 0.0;
         Scalar x_n2_left = 0.0;
         Scalar x_co2_right = 0.0;
         Scalar x_n2_right = 0.0;
         Scalar x_h2_left = 0.0;
         Scalar x_h2_right = 0.0;
         Scalar i = 0.0;
         Scalar j = 0.0;
         for (const auto& element : elements(gridGeometry.gridView()))
         {
             auto fvGeometry = localView(gridGeometry);
             fvGeometry.bindElement(element);

             auto elemVolVars = localView(gridVariables.curGridVolVars());
             elemVolVars.bind(element, fvGeometry, curSol);
             for (auto&& scv : scvs(fvGeometry))
             {
                 const auto globalPos = scv.dofPosition();

                 if (globalPos[0] < 0.5)
                 {
                     x_co2_left += elemVolVars[scv].moleFraction(FluidSystem::CO2Idx);
                     x_n2_left += elemVolVars[scv].moleFraction(FluidSystem::N2Idx);
                     x_h2_left += elemVolVars[scv].moleFraction(FluidSystem::H2Idx);
                     i +=1;
                 }
                 else
                 {
                     x_co2_right += elemVolVars[scv].moleFraction(FluidSystem::CO2Idx);
                     x_n2_right += elemVolVars[scv].moleFraction(FluidSystem::N2Idx);
                     x_h2_right += elemVolVars[scv].moleFraction(FluidSystem::H2Idx);
                     j +=1;
                 }
             }
         }
         x_co2_left /= i;
         x_n2_left /= i;
         x_h2_left /= i;
         x_co2_right /= j;
         x_n2_right /= j;
         x_h2_right /= j;

         //do a gnuplot
         x_.push_back(time); // in seconds
         y1_.push_back(x_n2_left);
         y2_.push_back(x_n2_right);
         y3_.push_back(x_co2_left);
         y4_.push_back(x_co2_right);
         y5_.push_back(x_h2_left);
         y6_.push_back(x_h2_right);

         gnuplot_.resetPlot();
         gnuplot_.setXRange(0, std::max(time, 72000.0));
         gnuplot_.setYRange(0.4, 0.6);
         gnuplot_.setXlabel("time [s]");
         gnuplot_.setYlabel("mole fraction mol/mol");
         gnuplot_.addDataSetToPlot(x_, y1_, "N2_left.dat", "w l t 'N_2 left'");
         gnuplot_.addDataSetToPlot(x_, y2_, "N2_right.dat", "w l t 'N_2 right'");
         gnuplot_.plot("mole_fraction_N2");

         gnuplot2_.resetPlot();
         gnuplot2_.setXRange(0, std::max(time, 72000.0));
         gnuplot2_.setYRange(0.0, 0.6);
         gnuplot2_.setXlabel("time [s]");
         gnuplot2_.setYlabel("mole fraction mol/mol");
         gnuplot2_.addDataSetToPlot(x_, y3_, "CO2_left.dat", "w l t 'CO_2 left'");
         gnuplot2_.addDataSetToPlot(x_, y4_, "CO2_right.dat", "w l t 'CO_2 right'");
         gnuplot2_.plot("mole_fraction_C02");

         gnuplot3_.resetPlot();
         gnuplot3_.setXRange(0, std::max(time, 72000.0));
         gnuplot3_.setYRange(0.0, 0.6);
         gnuplot3_.setXlabel("time [s]");
         gnuplot3_.setYlabel("mole fraction mol/mol");
         gnuplot3_.addDataSetToPlot(x_, y5_, "H2_left.dat", "w l t 'H_2 left'");
         gnuplot3_.addDataSetToPlot(x_, y6_, "H2_right.dat", "w l t 'H_2 right'");
         gnuplot3_.plot("mole_fraction_H2");
     }

private:
    GnuplotInterface<Scalar> gnuplot_;
    GnuplotInterface<Scalar> gnuplot2_;
    GnuplotInterface<Scalar> gnuplot3_;

    std::vector<Scalar> x_;
    std::vector<Scalar> y1_;
    std::vector<Scalar> y2_;
    std::vector<Scalar> y3_;
    std::vector<Scalar> y4_;
    std::vector<Scalar> y5_;
    std::vector<Scalar> y6_;
};

}




int main(int argc, char** argv)
{
    using namespace Dumux;

    // define the type tag for this problem
    using MomentumTypeTag = Properties::TTag::MaxwellStefanNCTestMomentum;
    using MassTypeTag = Properties::TTag::MaxwellStefanNCTestMass;

    // initialize MPI, finalize is done automatically on exit
    const auto& mpiHelper = Dune::MPIHelper::instance(argc, argv);

    // print dumux start message
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/true);

    // parse command line arguments and input file
    Parameters::init(argc, argv);

    // try to create a grid (from the given grid file or the input file)
    GridManager<GetPropType<MomentumTypeTag, Properties::Grid>> gridManager;
    gridManager.init();

    ////////////////////////////////////////////////////////////
    // run instationary non-linear problem on this grid
    ////////////////////////////////////////////////////////////

    // we compute on the leaf grid view
    const auto& leafGridView = gridManager.grid().leafGridView();

    // create the finite volume grid geometry
    using MomentumGridGeometry = GetPropType<MomentumTypeTag, Properties::GridGeometry>;
    auto momentumGridGeometry = std::make_shared<MomentumGridGeometry>(leafGridView);
    momentumGridGeometry->update();

    using MassGridGeometry = GetPropType<MassTypeTag, Properties::GridGeometry>;
    auto massGridGeometry = std::make_shared<MassGridGeometry>(leafGridView);
    massGridGeometry->update();

    // the coupling manager
    using Traits = MultiDomainTraits<MomentumTypeTag, MassTypeTag>;
    using CouplingManager = StaggeredFreeFlowCouplingManager<Traits>;

    auto couplingManager = std::make_shared<CouplingManager>();

    // the problem (boundary conditions)
    using MomentumProblem = GetPropType<MomentumTypeTag, Properties::Problem>;
    auto momentumProblem = std::make_shared<MomentumProblem>(momentumGridGeometry, couplingManager);

    using MassProblem = GetPropType<MassTypeTag, Properties::Problem>;
    auto massProblem = std::make_shared<MassProblem>(massGridGeometry, couplingManager);

    // get some time loop parameters
    using Scalar = typename Traits::Scalar;
    const auto tEnd = getParam<Scalar>("TimeLoop.TEnd");
    const auto maxDt = getParam<Scalar>("TimeLoop.MaxTimeStepSize");
    auto dt = getParam<Scalar>("TimeLoop.DtInitial");

    // instantiate time loop
    auto timeLoop = std::make_shared<CheckPointTimeLoop<Scalar>>(0, dt, tEnd);
    timeLoop->setMaxTimeStepSize(maxDt);

    // the solution vector
    constexpr auto momentumIdx = Dune::index_constant<0>();
    constexpr auto massIdx = Dune::index_constant<1>();
    using SolutionVector = typename Traits::SolutionVector;
    SolutionVector x;
    x[momentumIdx].resize(momentumGridGeometry->numDofs());
    x[massIdx].resize(massGridGeometry->numDofs());
    momentumProblem->applyInitialSolution(x[momentumIdx]);
    massProblem->applyInitialSolution(x[massIdx]);
    auto xOld = x;

    // the grid variables
    using MomentumGridVariables = GetPropType<MomentumTypeTag, Properties::GridVariables>;
    auto momentumGridVariables = std::make_shared<MomentumGridVariables>(momentumProblem, momentumGridGeometry);

    using MassGridVariables = GetPropType<MassTypeTag, Properties::GridVariables>;
    auto massGridVariables = std::make_shared<MassGridVariables>(massProblem, massGridGeometry);

    couplingManager->init(momentumProblem, massProblem, std::make_tuple(momentumGridVariables, massGridVariables), x, xOld);
    momentumGridVariables->init(x[momentumIdx]);
    massGridVariables->init(x[massIdx]);

    // initialize the vtk output module
    using IOFields = GetPropType<MassTypeTag, Properties::IOFields>;
    VtkOutputModule vtkWriter(*massGridVariables, x[massIdx], massProblem->name());
    IOFields::initOutputModule(vtkWriter); // Add model specific output fields
    vtkWriter.addVelocityOutput(std::make_shared<NavierStokesVelocityOutput<MassGridVariables>>());

    vtkWriter.write(0.0);

    using Assembler = MultiDomainFVAssembler<Traits, CouplingManager, DiffMethod::numeric>;
    auto assembler = std::make_shared<Assembler>(std::make_tuple(momentumProblem, massProblem),
                                                                 std::make_tuple(momentumGridGeometry, massGridGeometry),
                                                                 std::make_tuple(momentumGridVariables, massGridVariables),
                                                                 couplingManager,
                                                                 timeLoop, xOld);
    // the linear solver
    using LinearSolver = Dumux::UMFPackBackend;
    auto linearSolver = std::make_shared<LinearSolver>();

    // the non-linear solver
    using NewtonSolver = MultiDomainNewtonSolver<Assembler, LinearSolver, CouplingManager>;
    NewtonSolver nonLinearSolver(assembler, linearSolver, couplingManager);

    //! set some check points for the time loop
    timeLoop->setPeriodicCheckPoint(tEnd/5.0);

    PlotConcentration<Scalar> plotConcentration;

    // time loop
    timeLoop->start(); do
    {
        // solve the non-linear system with time step control
        nonLinearSolver.solve(x, *timeLoop);

        // make the new solution the old solution
        xOld = x;
        momentumGridVariables->advanceTimeStep();
        massGridVariables->advanceTimeStep();

        static const bool plotOutput = getParam<bool>("Problem.PlotOutput", false);
        if (plotOutput)
            plotConcentration.plotComponentsOverTime(x[massIdx], *massGridVariables, timeLoop->time()+timeLoop->timeStepSize());

        // advance to the time loop to the next step
        timeLoop->advanceTimeStep();

        // write vtk output on check points
        if (timeLoop->isCheckPoint())
            vtkWriter.write(timeLoop->time());

        // report statistics of this time step
        timeLoop->reportTimeStep();

        // set new dt as suggested by newton solver
        timeLoop->setTimeStepSize(nonLinearSolver.suggestTimeStepSize(timeLoop->timeStepSize()));

    } while (!timeLoop->finished());

    timeLoop->finalize(leafGridView.comm());

    ////////////////////////////////////////////////////////////
    // finalize, print dumux message to say goodbye
    ////////////////////////////////////////////////////////////

    // print dumux end message
    if (mpiHelper.rank() == 0)
    {
        Parameters::print();
        DumuxMessage::print(/*firstCall=*/false);
    }

    return 0;
} // end main