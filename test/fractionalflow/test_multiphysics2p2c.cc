// $Id$

#include "config.h"
// std lib includes:
#include <iostream>
#include <iomanip>
// dune stuff:
#include <dune/grid/sgrid.hh>
#include <dune/istl/io.hh>
#include <dune/common/timer.hh>
// dumux time discretization:
#include "dumux/timedisc/timeloop.hh"
#include "dumux/timedisc/expliciteulerstep.hh"
// material properties:
#include "dumux/material/fluids/water_air.hh"
#include <dumux/material/matrixproperties.hh>
#include <dumux/material/twophaserelations.hh>
// problem definition and model:
#include "dumux/transport/problems/testproblem_2p2c.hh"
#include "dumux/transport/fv/multiphysics2p2c.hh"

/***********************************************
 * Jochen Fritz, 2009                          *
 * Test application to class Multiphysics2p2c  *
 ***********************************************/

int main(int argc, char** argv)
{
  try{
    // define the problem dimensions
    const int dim=3;

//    // create a grid object
    typedef double NumberType;
    typedef Dune::SGrid<dim,dim> GridType;
    typedef GridType::LevelGridView GridView;
    typedef Dune::FieldVector<GridType::ctype,dim> FieldVector;
    Dune::FieldVector<int,dim> N(10);
    FieldVector L(0);
    FieldVector H(10);
    GridType grid(N,L,H);
    GridView gridview(grid.levelView(0));

    double tStart = 0;
    double tEnd = 3e4;
    int modulo = 1;
    double cFLFactor = 0.7;

    Dune::Liq_WaterAir wetmat;
    Dune::Gas_WaterAir nonwetmat;
    Dune::HomogeneousSoil<GridType, NumberType> soil;

    Dune::TwoPhaseRelations<GridType, NumberType> materialLaw(soil, wetmat, nonwetmat);

    Dune::VariableClass2p2c<GridView,NumberType> var(gridview);

    typedef Dune::Testproblem_2p2c<GridView, NumberType> TransProb;
    TransProb problem(gridview, var, wetmat, nonwetmat, soil, grid.maxLevel(), materialLaw, false);

    typedef Dune::Multiphysics2p2c<GridView, NumberType> ModelType;
    ModelType model(gridview, problem);

    Dune::ExplicitEulerStep<GridType, ModelType> timestep;
    Dune::TimeLoop<GridType, ModelType > timeloop(tStart, tEnd, "mp", modulo, cFLFactor, 1e100, 1e100, timestep);

    Dune::Timer timer;
    timer.reset();
    timeloop.execute(model);
    std::cout << "timeloop.execute took " << timer.elapsed() << " seconds" << std::endl;

    return 0;
  }
  catch (Dune::Exception &e){
    std::cerr << "Dune reported error: " << e << std::endl;
    return 1;
  }
  catch (...){
    std::cerr << "Unknown exception thrown!" << std::endl;
    return 1;
  }
}
