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
// ## The main program (`main.cc`)
// This file contains the main program flow. In this example, we use a single-phase
// Pore-Network-Model to evaluate the upscaled Darcy permeability of a given network.
// [[content]]
// ### Includes
// [[details]] includes
// [[codeblock]]
#include <config.h>

#include <iostream>

#include <dumux/common/properties.hh> // for GetPropType
#include <dumux/common/parameters.hh> // for getParam

#include <dumux/linear/seqsolverbackend.hh> // for ILU0BiCGSTABBackend
#include <dumux/linear/pdesolver.hh>        // for LinearPDESolver
#include <dumux/assembly/fvassembler.hh>
#include <dumux/assembly/diffmethod.hh>

#include <dumux/io/vtkoutputmodule.hh>
#include <dumux/io/grid/gridmanager_yasp.hh>

#include <dumux/io/grid/porenetwork/gridmanager.hh> // for pore-network grid
#include <dumux/porenetworkflow/common/pnmvtkoutputmodule.hh>

#include "upscalinghelper.hh"
#include "properties.hh"
// [[/codeblock]]
// [[/details]]
//
// ### Beginning of the main function
// [[codeblock]]
int main(int argc, char** argv) try
{
    using namespace Dumux;

    // We parse the command line arguments.
    Parameters::init(argc, argv);

    // Convenience alias for the type tag of the problem.
    using TypeTag = Properties::TTag::PNMUpscaling;
    // [[/codeblock]]

    // ### Create the grid and the grid geometry
    // [[codeblock]]
    // The grid manager can be used to create a grid from the input file
    using GridManager = Dumux::PoreNetworkGridManager<3>;
    GridManager gridManager;
    gridManager.init();

    // We compute on the leaf grid view.
    const auto& leafGridView = gridManager.grid().leafGridView();

    // instantiate the grid geometry
    using GridGeometry = GetPropType<TypeTag, Properties::GridGeometry>;
    auto gridGeometry = std::make_shared<GridGeometry>(leafGridView);
    auto gridData = gridManager.getGridData();
    gridGeometry->update(*gridData);
    // [[/codeblock]]

    // ### Initialize the problem and grid variables
    // [[codeblock]]
    using SpatialParams = GetPropType<TypeTag, Properties::SpatialParams>;
    auto spatialParams = std::make_shared<SpatialParams>(gridGeometry);
    using Problem = GetPropType<TypeTag, Properties::Problem>;
    auto problem = std::make_shared<Problem>(gridGeometry, spatialParams);

    // instantiate and initialize the discrete and exact solution vectors
    using SolutionVector = GetPropType<TypeTag, Properties::SolutionVector>;
    SolutionVector x(gridGeometry->numDofs());

    // instantiate and initialize the grid variables
    using GridVariables = GetPropType<TypeTag, Properties::GridVariables>;
    auto gridVariables = std::make_shared<GridVariables>(problem, gridGeometry);
    gridVariables->init(x);
    // [[/codeblock]]

    // ### Initialize VTK output
    using VtkOutputFields = GetPropType<TypeTag, Properties::IOFields>;
    PNMVtkOutputModule<TypeTag> vtkWriter(*gridVariables, x, problem->name());
    VtkOutputFields::initOutputModule(vtkWriter);
    vtkWriter.addField(gridGeometry->poreVolume(), "poreVolume", PNMVtkOutputModule<TypeTag>::FieldType::vertex);
    vtkWriter.addField(gridGeometry->throatShapeFactor(), "throatShapeFactor", PNMVtkOutputModule<TypeTag>::FieldType::element);

    // ### Instantiate the solver
    // We use the `LinearPDESolver` class, which is instantiated on the basis
    // of an assembler and a linear solver. When the `solve` function of the
    // `LinearPDESolver` is called, it uses the assembler and linear
    // solver classes to assemble and solve the linear system around the provided
    // solution and stores the result therein.
    // [[codeblock]]
    using Assembler = FVAssembler<TypeTag, DiffMethod::analytic>;
    auto assembler = std::make_shared<Assembler>(problem, gridGeometry, gridVariables);

    using LinearSolver = UMFPackBackend;
    auto linearSolver = std::make_shared<LinearSolver>();
    LinearPDESolver<Assembler, LinearSolver> solver(assembler,  linearSolver);
    solver.setVerbose(false); // suppress output during solve()
    // [[/codeblock]]

    // ### Solution of the problem and error computation
    using GridView = typename GetPropType<TypeTag, Properties::GridGeometry>::GridView;
    const auto defaultDirections = GridView::dimensionworld == 3 ? std::vector<int>{0, 1, 2}
                                                                 : std::vector<int>{0, 1};
    const auto directions = getParam<std::vector<int>>("Problem.Directions", defaultDirections);

    // ### Helper class to evaluate the permeability
    const auto upscalinghelper = UpscalingHelper(*assembler);

    // [[/codeblock]]

    // This procedure is now repeated for the number of refinements as specified
    // in the input file.
    // [[codeblock]]
    for (int dimIdx : directions)
    {
        // reset the solution
        x = 0;

        // set the direction in which the pressure gradient will be applied
        problem->setDirection(dimIdx);

        // solve problem
        solver.solve(x);

        // write the vtu file for the given direction
        vtkWriter.write(dimIdx);

        // calculate the permeability
        upscalinghelper.doUpscaling(x, dimIdx);
    }
    // [[/codeblock]]


    // program end, return with 0 exit code (success)
    return 0;
}
// [[/codeblock]]
// ### Exception handling
// In this part of the main file we catch and print possible exceptions that could
// occur during the simulation.
// [[details]] error handler
catch (const Dumux::ParameterException &e)
{
    std::cerr << std::endl << e << " ---> Abort!" << std::endl;
    return 1;
}
catch (const Dune::DGFException & e)
{
    std::cerr << "DGF exception thrown (" << e <<
                 "). Most likely, the DGF file name is wrong "
                 "or the DGF file is corrupted, "
                 "e.g. missing hash at end of file or wrong number (dimensions) of entries."
                 << " ---> Abort!" << std::endl;
    return 2;
}
catch (const Dune::Exception &e)
{
    std::cerr << "Dune reported error: " << e << " ---> Abort!" << std::endl;
    return 3;
}
// [[/details]]
// [[/content]]