#include <config.h>

#include <iostream>
#include <algorithm>
#include <initializer_list>

#include <dune/common/version.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/fvector.hh>
#include <dune/geometry/multilineargeometry.hh>

#include <dumux/common/geometry/geometryintersection.hh>


#ifndef DOXYGEN
template<int dimworld = 3>
Dune::MultiLinearGeometry<double, 1, dimworld>
makeLine(std::initializer_list<Dune::FieldVector<double, dimworld>>&& c)
{
#if DUNE_VERSION_NEWER(DUNE_COMMON,2,6)
    return {Dune::GeometryTypes::line, c};
#else
    return {Dune::GeometryType(1), c};
#endif
}

template<int dimworld = 3>
bool testIntersection(const Dune::MultiLinearGeometry<double, dimworld, dimworld>& cube,
                      const Dune::MultiLinearGeometry<double, 1, dimworld>& line,
                      bool foundExpected = true)
{
    using Test = Dumux::GeometryIntersection<Dune::MultiLinearGeometry<double,dimworld,dimworld>,
                                             Dune::MultiLinearGeometry<double,1,dimworld> >;
    typename Test::IntersectionType intersection;
    bool found = Test::intersection(cube, line, intersection);
    if (!found && foundExpected)
        std::cerr << "Failed detecting intersection with " << line.corner(0) << " " << line.corner(1) << std::endl;
    else if (found && foundExpected)
        std::cout << "Found intersection with " << line.corner(0) << " " << line.corner(1) << std::endl;
    else if (found && !foundExpected)
        std::cerr << "Found false positive: intersection with " << line.corner(0) << " " << line.corner(1) << std::endl;
    else if (!found && !foundExpected)
        std::cout << "No intersection with " << line.corner(0) << " " << line.corner(1) << std::endl;
    return (found == foundExpected);
}
#endif

int main (int argc, char *argv[]) try
{
    // maybe initialize mpi
    Dune::MPIHelper::instance(argc, argv);

    constexpr int dimworld = 3;
    constexpr int dim = 3;

    std::vector<Dune::FieldVector<double, dimworld>> cubeCorners({
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}, {1.0, 1.0, 1.0}
    });

    Dune::MultiLinearGeometry<double, dim, dimworld>
#if DUNE_VERSION_NEWER(DUNE_COMMON,2,6)
        cube(Dune::GeometryTypes::cube(dimworld), cubeCorners);
#else
        Dune::GeometryType geomType; geomType.makeCube(3);
        cube(geomType, cubeCorners);
#endif


    // collect returns to determine exit code
    std::vector<bool> returns;

    // the tests
    returns.push_back(testIntersection(cube, makeLine({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}})));

    returns.push_back(testIntersection(cube, makeLine({{1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 0.0, 0.0}, {1.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}})));

    returns.push_back(testIntersection(cube, makeLine({{0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.0, 1.0, 0.0}, {0.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}})));

    returns.push_back(testIntersection(cube, makeLine({{1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 1.0, 0.0}, {1.0, 0.0, 0.0}})));

    returns.push_back(testIntersection(cube, makeLine({{0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.0, 0.0, 1.0}, {0.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}})));

    returns.push_back(testIntersection(cube, makeLine({{1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 0.0, 1.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}})));

    returns.push_back(testIntersection(cube, makeLine({{0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.0, 1.0, 1.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.0, 1.0, 1.0}, {1.0, 1.0, 1.0}})));

    returns.push_back(testIntersection(cube, makeLine({{1.0, 1.0, 1.0}, {1.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 1.0, 1.0}, {1.0, 0.0, 1.0}})));

    returns.push_back(testIntersection(cube, makeLine({{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(cube, makeLine({{0.5, 0.5, 0.5}, {0.5, 0.5, -2.0}})));

    returns.push_back(testIntersection(cube, makeLine({{0.5, 0.5, 0.0}, {0.5, 0.5, -2.0}}), false));
    returns.push_back(testIntersection(cube, makeLine({{1.0, 1.0, 1.0}, {2.0, 2.0, 2.0}}), false));

    // determine the exit code
    if (std::any_of(returns.begin(), returns.end(), [](bool i){ return !i; }))
        return 1;

    std::cout << "All tests passed!" << std::endl;

    return 0;
}
// //////////////////////////////////
//   Error handler
// /////////////////////////////////
catch (const Dune::Exception& e) {
    std::cout << e << std::endl;
    return 1;
}
