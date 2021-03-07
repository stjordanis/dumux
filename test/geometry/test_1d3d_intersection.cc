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

    for (const double scaling : {1.0, 1e3, 1e12, 1e-12})
    {
        std::cout << "Test with scaling " << scaling << std::endl;

        using Points = std::vector<Dune::FieldVector<double, dimworld>>;
        using Geo = Dune::MultiLinearGeometry<double, dim, dimworld>;

        // test tetrahedron-line intersections
        std::cout << "test tetrahedron-line intersections" << std::endl;

        auto cornersTetrahedron = Points({{0.0, 0.0, 0.0}, {1.0*scaling, 0.0, 0.0}, {0.0, 1.0*scaling, 0.0}, {0.0, 0.0, 1.0*scaling}});
        auto tetrahedron = Geo(Dune::GeometryTypes::tetrahedron, cornersTetrahedron);

        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.0}, {1.0*scaling, 0.0, 0.0}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{1.0*scaling, 0.0, 0.0}, {0.0, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 1.0*scaling, 0.0}, {0.0, 0.0, 0.0}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{1.0*scaling, 0.0, 0.0}, {0.0, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 1.0*scaling, 0.0}, {0.0, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.5*scaling}, {0.5*scaling, 0.0, 0.5*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.5*scaling}, {0.0, 0.5*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.5*scaling, 0.0, 0.5*scaling}, {0.0, 0.5*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 1.0*scaling}, {0.5*scaling, 0.0, 0.0}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 1.0*scaling}, {0.0, 0.5*scaling, 0.0}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 1.0*scaling}, {0.5*scaling, 0.5*scaling, 0.0}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.0}, {1.0*scaling, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.25*scaling, 0.25*scaling, 0.0}, {0.25*scaling, 0.25*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{-1.0*scaling, 0.25*scaling, 0.5*scaling}, {1.0*scaling, 0.25*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{1.0*scaling, 1.0*scaling, 1.0*scaling}, {-1.0*scaling, -1.0*scaling, -1.0*scaling}})));
        returns.push_back(testIntersection(tetrahedron, makeLine({{1.5*scaling, 0.0, 0.5*scaling}, {0.0, 1.5*scaling, 0.5*scaling}}), false));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, -1.0*scaling}}), false));
        returns.push_back(testIntersection(tetrahedron, makeLine({{1.0*scaling, 1.0*scaling, 0.0}, {0.0, 0.0, 2.0*scaling}}), false));
        returns.push_back(testIntersection(tetrahedron, makeLine({{1.0*scaling, 0.0, 0.1*scaling}, {0.0, 1.0*scaling, 0.1*scaling}}), false));
        returns.push_back(testIntersection(tetrahedron, makeLine({{0.0, 0.0, -0.1*scaling}, {1.0*scaling, 1.0*scaling, -0.1*scaling}}), false));

        // test hexahedron-line intersections
        std::cout << "test hexahedron-line intersections" << std::endl;

        auto cornersHexahedron = Points({{0.0, 0.0, 0.0}, {1.0*scaling, 0.0, 0.0}, {0.0, 1.0*scaling, 0.0}, {1.0*scaling, 1.0*scaling, 0.0},
                                         {0.0, 0.0, 1.0*scaling}, {1.0*scaling, 0.0, 1.0*scaling}, {0.0, 1.0*scaling, 1.0*scaling}, {1.0*scaling, 1.0*scaling, 1.0*scaling}});
        auto hexahedron = Geo(Dune::GeometryTypes::hexahedron, cornersHexahedron);

        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 0.0}, {1.0*scaling, 0.0, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 0.0}, {0.0, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 0.0, 0.0}, {0.0, 0.0, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 0.0, 0.0}, {1.0*scaling, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 0.0, 0.0}, {1.0*scaling, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0*scaling, 0.0}, {0.0, 0.0, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0*scaling, 0.0}, {0.0, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0*scaling, 0.0}, {1.0*scaling, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 1.0*scaling, 0.0}, {1.0*scaling, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 1.0*scaling, 0.0}, {0.0, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 1.0*scaling, 0.0}, {1.0*scaling, 0.0, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 1.0*scaling}, {1.0*scaling, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 1.0*scaling}, {0.0, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 1.0*scaling}, {0.0, 0.0, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 0.0, 1.0*scaling}, {0.0, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 0.0, 1.0*scaling}, {1.0*scaling, 0.0, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 0.0, 1.0*scaling}, {1.0*scaling, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0*scaling, 1.0*scaling}, {0.0, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0*scaling, 1.0*scaling}, {0.0, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 1.0*scaling, 1.0*scaling}, {1.0*scaling, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 1.0*scaling, 1.0*scaling}, {1.0*scaling, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 1.0*scaling, 1.0*scaling}, {0.0, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.0*scaling, 1.0*scaling, 1.0*scaling}, {1.0*scaling, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 0.0}, {1.0*scaling, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.5*scaling, 0.5*scaling, 0.5*scaling}, {0.5*scaling, 0.5*scaling, -2.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.5*scaling, 0.0, 0.5*scaling}, {0.5*scaling, 1.0*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.5*scaling, 0.5*scaling}, {1.0*scaling, 0.5*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.5*scaling, 0.5*scaling, 0.0}, {0.5*scaling, 0.5*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 2.0*scaling}, {1.0*scaling, 1.0*scaling, 2.0*scaling}}), false));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, 0.0, 1.1*scaling}, {1.0*scaling, 1.0*scaling, 1.1*scaling}}), false));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.1*scaling, 1.1*scaling, 0.0}, {1.1*scaling, 1.1*scaling, 1.0*scaling}}), false));
        returns.push_back(testIntersection(hexahedron, makeLine({{1.1*scaling, 0.0, 0.0}, {1.1*scaling, 1.0*scaling, 1.0*scaling}}), false));
        returns.push_back(testIntersection(hexahedron, makeLine({{0.0, -0.1*scaling, 0.0}, {1.0*scaling, -0.1*scaling, 0.0*scaling}}), false));

        // test pyramid-line intersections
        std::cout << "test pyramid-line intersections" << std::endl;

        auto cornersPyramid = Points({{0.0, 0.0, 0.0}, {1.0*scaling, 0.0, 0.0}, {0.0, 1.0*scaling, 0.0}, {1.0*scaling, 1.0*scaling, 0.0}, {0.5*scaling, 0.5*scaling, 1.0*scaling}});
        auto pyramid = Geo(Dune::GeometryTypes::pyramid, cornersPyramid);

        returns.push_back(testIntersection(pyramid, makeLine({{0.0, 0.0, 0.0}, {1.0*scaling, 0.0, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{1.0*scaling, 0.0, 0.0}, {1.0*scaling, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{1.0*scaling, 1.0*scaling, 0.0}, {0.0, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.0, 1.0*scaling, 0.0}, {0.0, 0.0, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.5*scaling, 0.5*scaling, 1.0*scaling}, {0.0, 0.0, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.5*scaling, 0.5*scaling, 1.0*scaling}, {1.0*scaling, 0.0, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.5*scaling, 0.5*scaling, 1.0*scaling}, {0.0, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.5*scaling, 0.5*scaling, 1.0*scaling}, {1.0*scaling, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.5*scaling, 0.5*scaling, 1.0*scaling}, {0.5*scaling, 0.5*scaling, 0.0}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.25*scaling, 0.25*scaling, 0.5*scaling}, {0.75*scaling, 0.25*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.75*scaling, 0.25*scaling, 0.5*scaling}, {0.75*scaling, 0.75*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.75*scaling, 0.75*scaling, 0.5*scaling}, {0.25*scaling, 0.75*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.25*scaling, 0.75*scaling, 0.5*scaling}, {0.25*scaling, 0.25*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(pyramid, makeLine({{0.0, 0.0, 1.0*scaling}, {1.0*scaling, 0.0, 1.0*scaling}}), false));
        returns.push_back(testIntersection(pyramid, makeLine({{0.0, 0.0, 1.0*scaling}, {0.0, 1.0*scaling, 1.0*scaling}}), false));
        returns.push_back(testIntersection(pyramid, makeLine({{0.0, 0.0, -0.1*scaling}, {1.0*scaling, 1.0*scaling, -0.1*scaling}}), false));
        returns.push_back(testIntersection(pyramid, makeLine({{0.0, 1.1*scaling, 0.0}, {1.0*scaling, 1.1*scaling, 0.0}}), false));
        returns.push_back(testIntersection(pyramid, makeLine({{0.4*scaling, 0.0, 1.0*scaling}, {0.4*scaling, 1.0*scaling, 1.0*scaling}}), false));

        // test prism-line intersections
        std::cout << "test prism-line intersections" << std::endl;

        auto cornersPrism = Points({{0.0, 0.0, 0.0}, {1.0*scaling, 0.0, 0.0}, {0.0, 1.0*scaling, 0.0},
                                    {0.0, 0.0, 1.0*scaling}, {1.0*scaling, 0.0, 1.0*scaling}, {0.0, 1.0*scaling, 1.0*scaling}});
        auto prism = Geo(Dune::GeometryTypes::prism, cornersPrism);

        returns.push_back(testIntersection(prism, makeLine({{0.0, 0.0, 0.0}, {1.0*scaling, 0.0, 0.0}})));
        returns.push_back(testIntersection(prism, makeLine({{1.0*scaling, 0.0, 0.0}, {0.0, 1.0*scaling, 0.0}})));
        returns.push_back(testIntersection(prism, makeLine({{0.0, 1.0*scaling, 0.0}, {0.0, 0.0, 0.0}})));
        returns.push_back(testIntersection(prism, makeLine({{0.0, 0.0, 1.0*scaling}, {1.0*scaling, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{1.0*scaling, 0.0, 1.0*scaling}, {0.0, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{0.0, 1.0*scaling, 1.0*scaling}, {0.0, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{1.0*scaling, 0.0, 0.0}, {1.0*scaling, 0.0, 1.0*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{0.0, 1.0*scaling, 0.0}, {0.0, 1.0*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{0.25*scaling, 0.25*scaling, 0.0}, {0.25*scaling, 0.25*scaling, 1.0*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{0.0, 0.0, 0.5*scaling}, {1.0*scaling, 0.0, 0.5*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{1.0*scaling, 0.0, 0.5*scaling}, {0.0, 1.0*scaling, 0.5*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{0.0, 1.0*scaling, 0.5*scaling}, {0.0, 0.0, 0.5*scaling}})));
        returns.push_back(testIntersection(prism, makeLine({{1.0*scaling, 1.0*scaling, 0.0}, {1.0*scaling, 1.0*scaling, 1.0*scaling}}), false));
        returns.push_back(testIntersection(prism, makeLine({{2.0*scaling, 0.0, 0.5*scaling}, {0.0, 2.0*scaling, 0.5*scaling}}), false));
        returns.push_back(testIntersection(prism, makeLine({{1.1*scaling, 0.0, 0.0}, {1.1*scaling, 0.0, 1.0*scaling}}), false));
        returns.push_back(testIntersection(prism, makeLine({{-0.1*scaling, 0.0, 1.0*scaling}, {-0.1*scaling, 1.0*scaling, 1.0*scaling}}), false));
        returns.push_back(testIntersection(prism, makeLine({{1.0*scaling, 0.0, 1.1*scaling}, {0.0, 1.0*scaling, 1.1*scaling}}), false));
    }

    // determine the exit code
    if (std::any_of(returns.begin(), returns.end(), std::logical_not<bool>{}))
        return 1;

    std::cout << "\n++++++++++++++++++++++\n"
              << "All tests passed!"
              << "\n++++++++++++++++++++++" << std::endl;

    return 0;
}
