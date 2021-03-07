#include <config.h>

#include <iostream>
#include <algorithm>
#include <initializer_list>

#include <dune/common/exceptions.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/fvector.hh>
#include <dune/geometry/multilineargeometry.hh>

#include <dumux/geometry/geometryintersection.hh>


#ifndef DOXYGEN
template<int dimworld = 3>
Dune::MultiLinearGeometry<double, 1, dimworld>
makeLine(std::initializer_list<Dune::FieldVector<double, dimworld>>&& c)
{
    return {Dune::GeometryTypes::line, c};
}

template<int dimworld = 3>
bool testIntersection(const Dune::MultiLinearGeometry<double, dimworld, dimworld>& polyhedron,
                      const Dune::MultiLinearGeometry<double, 1, dimworld>& line,
                      bool foundExpected = true)
{
    using Test = Dumux::GeometryIntersection<Dune::MultiLinearGeometry<double,dimworld,dimworld>,
                                             Dune::MultiLinearGeometry<double,1,dimworld> >;
    typename Test::Intersection intersection;
    bool found = Test::intersection(polyhedron, line, intersection);
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

int main (int argc, char *argv[])
{
    // maybe initialize mpi
    Dune::MPIHelper::instance(argc, argv);

    constexpr int dimworld = 3;
    constexpr int dim = 3;

    // collect returns to determine exit code
    std::vector<bool> returns;

    using Points = std::vector<Dune::FieldVector<double, dimworld>>;
    using Geo = Dune::MultiLinearGeometry<double, dim, dimworld>;

    // test tetrahedron-line intersections
    std::cout << "test tetrahedron-line intersections" << std::endl;

    auto cornersTetrahedron = Points({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}});
    auto tetrahedron = Geo(Dune::GeometryTypes::tetrahedron, cornersTetrahedron);

    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.5}, {0.5, 0.0, 0.5}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.5}, {0.0, 0.5, 0.5}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.5, 0.0, 0.5}, {0.0, 0.5, 0.5}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 1.0}, {0.5, 0.0, 0.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 1.0}, {0.0, 0.5, 0.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 1.0}, {0.5, 0.5, 0.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.25, 0.25, 0.0}, {0.25, 0.25, 1.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{-1.0, 0.25, 0.5}, {1.0, 0.25, 0.5}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{1.0, 1.0, 1.0}, {-1.0, -1.0, -1.0}})));
    returns.push_back(testIntersection(tetrahedron, makeLine({{1.5, 0.0, 0.5}, {0.0, 1.5, 0.5}}), false));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, -1.0}}), false));
    returns.push_back(testIntersection(tetrahedron, makeLine({{1.0, 1.0, 0.0}, {0.0, 0.0, 2.0}}), false));
    returns.push_back(testIntersection(tetrahedron, makeLine({{1.0, 0.0, 0.1}, {0.0, 1.0, 0.1}}), false));
    returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, -0.1}, {1.0, 1.0, -0.1}}), false));

    // test hexahedron-line intersections
    std::cout << "test hexahedron-line intersections" << std::endl;

    auto cornersHexahedron = Points({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0},
                                     {0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}, {1.0, 1.0, 1.0}});
    auto hexahedron = Geo(Dune::GeometryTypes::hexahedron, cornersHexahedron);

    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 0.0, 0.0}, {1.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0, 0.0}, {0.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 1.0, 0.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 1.0}, {0.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 0.0, 1.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0, 1.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0, 1.0}, {1.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 1.0, 1.0}, {1.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.0, 1.0, 1.0}, {1.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.5, 0.5, 0.5}, {0.5, 0.5, -2.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.5, 0.0, 0.5}, {0.5, 1.0, 0.5}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.5, 0.5}, {1.0, 0.5, 0.5}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.5, 0.5, 0.0}, {0.5, 0.5, 1.0}})));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 2.0}, {1.0, 1.0, 2.0}}), false));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 1.1}, {1.0, 1.0, 1.1}}), false));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.1, 1.1, 0.0}, {1.1, 1.1, 1.0}}), false));
    returns.push_back(testIntersection(hexahedron, makeLine({{1.1, 0.0, 0.0}, {1.1, 1.0, 1.0}}), false));
    returns.push_back(testIntersection(hexahedron, makeLine({{0.0, -0.1, 0.0}, {1.0, -0.1, 0.0}}), false));

    // test pyramid-line intersections
    std::cout << "test pyramid-line intersections" << std::endl;

    auto cornersPyramid = Points({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {0.5, 0.5, 1.0}});
    auto pyramid = Geo(Dune::GeometryTypes::pyramid, cornersPyramid);

    returns.push_back(testIntersection(pyramid, makeLine({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.5, 0.5, 1.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.5, 0.5, 1.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.5, 0.5, 1.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.5, 0.5, 1.0}, {1.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.5, 0.5, 1.0}, {0.5, 0.5, 0.0}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.25, 0.25, 0.5}, {0.75, 0.25, 0.5}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.75, 0.25, 0.5}, {0.75, 0.75, 0.5}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.75, 0.75, 0.5}, {0.25, 0.75, 0.5}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.25, 0.75, 0.5}, {0.25, 0.25, 0.5}})));
    returns.push_back(testIntersection(pyramid, makeLine({{0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}}), false));
    returns.push_back(testIntersection(pyramid, makeLine({{0.0, 0.0, 1.0}, {0.0, 1.0, 1.0}}), false));
    returns.push_back(testIntersection(pyramid, makeLine({{0.0, 0.0, -0.1}, {1.0, 1.0, -0.1}}), false));
    returns.push_back(testIntersection(pyramid, makeLine({{0.0, 1.1, 0.0}, {1.0, 1.1, 0.0}}), false));
    returns.push_back(testIntersection(pyramid, makeLine({{0.4, 0.0, 1.0}, {0.4, 1.0, 1.0}}), false));

    // test prism-line intersections
    std::cout << "test prism-line intersections" << std::endl;

    auto cornersPrism = Points({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
                                {0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}});
    auto prism = Geo(Dune::GeometryTypes::prism, cornersPrism);

    returns.push_back(testIntersection(prism, makeLine({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(prism, makeLine({{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}})));
    returns.push_back(testIntersection(prism, makeLine({{0.0, 1.0, 0.0}, {0.0, 0.0, 0.0}})));
    returns.push_back(testIntersection(prism, makeLine({{0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(prism, makeLine({{1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(prism, makeLine({{0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(prism, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(prism, makeLine({{1.0, 0.0, 0.0}, {1.0, 0.0, 1.0}})));
    returns.push_back(testIntersection(prism, makeLine({{0.0, 1.0, 0.0}, {0.0, 1.0, 1.0}})));
    returns.push_back(testIntersection(prism, makeLine({{0.25, 0.25, 0.0}, {0.25, 0.25, 1.0}})));
    returns.push_back(testIntersection(prism, makeLine({{0.0, 0.0, 0.5}, {1.0, 0.0, 0.5}})));
    returns.push_back(testIntersection(prism, makeLine({{1.0, 0.0, 0.5}, {0.0, 1.0, 0.5}})));
    returns.push_back(testIntersection(prism, makeLine({{0.0, 1.0, 0.5}, {0.0, 0.0, 0.5}})));
    returns.push_back(testIntersection(prism, makeLine({{1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}}), false));
    returns.push_back(testIntersection(prism, makeLine({{2.0, 0.0, 0.5}, {0.0, 2.0, 0.5}}), false));
    returns.push_back(testIntersection(prism, makeLine({{1.1, 0.0, 0.0}, {1.1, 0.0, 1.0}}), false));
    returns.push_back(testIntersection(prism, makeLine({{-0.1, 0.0, 1.0}, {-0.1, 1.0, 1.0}}), false));
    returns.push_back(testIntersection(prism, makeLine({{1.0, 0.0, 1.1}, {0.0, 1.0, 1.1}}), false));

    // determine the exit code
    if (std::any_of(returns.begin(), returns.end(), [](bool i){ return !i; }))
        return 1;

    std::cout << "All tests passed!" << std::endl;

    return 0;
}
