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
 * \ingroup BoundaryTests
 * \brief The spatial parameters class for the test problem using the 1p cc model.
 */

#ifndef DUMUX_CONVERGENCE_TEST_SPATIALPARAMS_HH
#define DUMUX_CONVERGENCE_TEST_SPATIALPARAMS_HH

#include <cmath>
#include <dumux/common/math.hh>
#include <dumux/common/parameters.hh>
#include <dune/common/fmatrix.hh>
#include <dumux/material/spatialparams/fv1p.hh>
#include "testcase.hh"

namespace Dumux {

/*!
 * \ingroup BoundaryTests
 * \brief The spatial parameters class for the test problem using the
 *        1p cc model.
 */
template<class GridGeometry, class Scalar>
class ConvergenceTestSpatialParams
: public FVSpatialParamsOneP<GridGeometry, Scalar,
                             ConvergenceTestSpatialParams<GridGeometry, Scalar>>
{
    using GridView = typename GridGeometry::GridView;
    using ParentType = FVSpatialParamsOneP<GridGeometry, Scalar,
                                           ConvergenceTestSpatialParams<GridGeometry, Scalar>>;

    using Element = typename GridView::template Codim<0>::Entity;
    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

    static constexpr int dimWorld = GridView::dimensionworld;
    using DimWorldMatrix = Dune::FieldMatrix<Scalar, dimWorld, dimWorld>;


public:
    // export permeability type
    using PermeabilityType = DimWorldMatrix;

    ConvergenceTestSpatialParams(std::shared_ptr<const GridGeometry> gridGeometry, const TestCase testCase)
    : ParentType(gridGeometry)
    , testCase_(testCase)
    , K_(0.0)
    {
        alphaBJ_ = getParam<Scalar>("Darcy.SpatialParams.AlphaBeaversJoseph");
        porosity_ = getParam<Scalar>("Darcy.SpatialParams.Porosity", 0.4);

        if(testCase_ == TestCase::BJSymmetrized || testCase_ == TestCase::NewICNonSymmetrized)
        {
            std::vector<Scalar> permeability = getParam<std::vector<Scalar>>("Darcy.SpatialParams.Permeability");
            K_[0][0] =            permeability[0];
            K_[1][1] =            permeability[1];
            K_[0][1] = K_[1][0] = permeability[2];
        }
        else
        {
            Scalar permeability = getParam<Scalar>("Darcy.SpatialParams.Permeability");
            K_[0][0] = permeability;
            K_[1][1] = permeability;
        }
    }

   /*!
     * \brief Function for defining the (intrinsic) permeability \f$[m^2]\f$.
     *
     * \param element The element
     * \param scv The sub control volume
     * \param elemSol The element solution vector
     * \return the intrinsic permeability
     */
    template<class SubControlVolume, class ElementSolution>
    PermeabilityType permeability(const Element& element,
                                  const SubControlVolume& scv,
                                  const ElementSolution& elemSol) const
    {
        PermeabilityType K(K_);

        if (testCase_ == TestCase::Schneider)
        {
            using std::cos;
            using std::sin;
            using std::exp;

            static constexpr Scalar c = 0.0;
            static constexpr Scalar omega = M_PI;

            const Scalar x = scv.center()[0];
            K[0][0] = 1.0;
            K[0][1] = -c/(2*omega) * sin(omega*x);
            K[1][0] = K[0][1];
            K[1][1] = exp(-2)*(1 + c*cos(omega*x));
        }

        return K;
    }

    /*! \brief Defines the porosity in [-].
     *
     * \param globalPos The global position
     */
    Scalar porosityAtPos(const GlobalPosition& globalPos) const
    { return porosity_; }

    /*! \brief Defines the Beavers-Joseph coefficient in [-].
     *
     * \param globalPos The global position
     */
    Scalar beaversJosephCoeffAtPos(const GlobalPosition& globalPos) const
    { return alphaBJ_; }

    Scalar epsInterfaceAtPos(const GlobalPosition& globalPos) const
    {
        static const Scalar epsInterface = getParam<Scalar>("Darcy.InterfaceParams.EpsInterface");
        return epsInterface;
    }

    Scalar factorNMomentumAtPos(const GlobalPosition& globalPos) const
    {
        static const Scalar N_s_bl = getParam<Scalar>("Darcy.InterfaceParams.N_s_bl");
        return N_s_bl;
    }

    Scalar factorNTangentialAtPos(const GlobalPosition& globalPos) const
    {
        static const Scalar N_1_bl = getParam<Scalar>("Darcy.InterfaceParams.N_1_bl");
        return N_1_bl;
    }

    PermeabilityType matrixNTangentialAtPos(const GlobalPosition& globalPos) const
    {
        static const std::vector<Scalar> M_bl = getParam<std::vector<Scalar>>("Darcy.InterfaceParams.M_bl");
        PermeabilityType M(0.0);
        M[0][0]= M_bl[0];
        M[1][1]= M_bl[1];

        return M;
    }

private:
    TestCase testCase_;
    PermeabilityType K_;
    Scalar alphaBJ_;
    Scalar porosity_;
};

} // end namespace Dumux

#endif
