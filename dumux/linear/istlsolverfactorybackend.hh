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
 * \ingroup Linear
 * \brief Provides a generic linear solver based on the ISTL that chooses the
 *        solver and preconditioner at runtime
 */

#ifndef DUMUX_LINEAR_ISTL_SOLVERFACTORYBACKEND_HH
#define DUMUX_LINEAR_ISTL_SOLVERFACTORYBACKEND_HH

#include <dune/common/version.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/parametertree.hh>

// preconditioners
#include <dune/istl/preconditioners.hh>
#include <dune/istl/paamg/amg.hh>

// solvers
#include <dune/istl/solvers.hh>
#include <dune/istl/solverfactory.hh>

#include <dumux/linear/solver.hh>
#include <dumux/linear/amgtraits.hh>
#include <dumux/linear/amgparallelhelpers.hh>

#if DUNE_VERSION_NEWER_REV(DUNE_ISTL,2,7,1)

namespace Dumux {

/*!
 * \ingroup Linear
 * \brief A linear solver using the dune-istl solver factory
 *        allowing choosing the solver and preconditioner
 *        at runtime.
 * \note the solvers are configured via the input file
 * \note Requires Dune version 2.7.1 or newer
 */
template <class Matrix, class Vector, class GridGeometry>
class IstlSolverFactoryBackend : public LinearSolver
{
    using GridView = typename GridGeometry::GridView;
    using AMGTraits =  AmgTraits<Matrix, Vector, GridGeometry>;
    using Grid = typename GridView::Grid;
    using LinearOperator = typename AMGTraits::LinearOperator;
    using ScalarProduct = typename AMGTraits::ScalarProduct;
    using VType = typename AMGTraits::VType;
    using Comm = typename AMGTraits::Comm;
    using BCRSMat = typename AMGTraits::LinearOperator::matrix_type;
    using DofMapper = typename AMGTraits::DofMapper;

public:
    //! translation table for solver parameters
    static std::vector<std::array<std::string,2> > dumuxToIstlSolverParams;

    /*!
     * \brief Construct the backend for the sequential case only
     *
     * \param paramGroup the parameter group for parameter lookup
     */
    IstlSolverFactoryBackend(const std::string& paramGroup = "")
    : paramGroup_(paramGroup)
    , firstCall_(true)
    {
        if (Dune::MPIHelper::getCollectiveCommunication().size() > 1)
            DUNE_THROW(Dune::InvalidStateException, "Using sequential constructor for parallel run. Use signature with gridView and dofMapper!");

        reset();
    }

    /*!
     * \brief Construct the backend for parallel or sequential runs
     *
     * \param gridView the grid view on which we are performing the multi-grid
     * \param dofMapper an index mapper for dof entities
     * \param paramGroup the parameter group for parameter lookup
     */
    IstlSolverFactoryBackend(const GridView& gridView,
                             const DofMapper& dofMapper,
                             const std::string& paramGroup = "")
    : paramGroup_(paramGroup)
    , parallelHelper_(std::make_unique<ParallelISTLHelper<GridView, AMGTraits>>(gridView, dofMapper))
    , firstCall_(true)
    {
        reset();
    }

    /*!
     * \brief Solve a linear system.
     *
     * \param A the matrix
     * \param x the seeked solution vector, containing the initial solution upon entry
     * \param b the right hand side vector
     */
    bool solve(Matrix& A, Vector& x, Vector& b)
    {
        int rank = 0;
        std::shared_ptr<Comm> comm;
        std::shared_ptr<LinearOperator> fop;
        std::shared_ptr<ScalarProduct> sp; // not used.

#if HAVE_MPI
        if constexpr (AMGTraits::isParallel)
            prepareLinearAlgebraParallel<AMGTraits>(A, b, comm, fop, sp, *phelper_, firstCall_);
        else
            prepareLinearAlgebraSequential<AMGTraits>(A, comm, fop, sp);
#else
        prepareLinearAlgebraSequential<AMGTraits>(A, comm, fop, sp);
#endif

        std::shared_ptr<Dune::InverseOperator<Vector, Vector>> solver;
        try{
            solver = Dune::getSolverFromFactory(fop, params_);
        }
        catch(Dune::Exception& e){
            std::cerr << "Could not create solver with factory" << std::endl;
            std::cerr << e.what() << std::endl;
            throw e;
        }

        // solve
        solver->apply(x, b, result_);

        firstCall_ = false;
        return result_.converged;
    }

    //! reset the linear solver factory
    void reset()
    {
        resetDefaultParameters();
        convertParameterTree_(paramGroup_);
        Dune::initSolverFactories<typename AMGTraits::LinearOperator>();
    }

    //! reset some defaults for the solver parameters
    void resetDefaultParameters()
    {
        params_["restart"] = "10";
        params_["maxit"] = "250";
        params_["reduction"] = "1e-13";
        params_["verbose"] = "0";
        params_["preconditioner.iterations"] = "1";
        params_["preconditioner.relaxation"] = "1.0";
    }

private:

    void convertParameterTree_(const std::string& paramGroup="")
    {
        auto linearSolverGroups = getParamSubGroups("LinearSolver", paramGroup);
        if (linearSolverGroups.empty()) // no linear solver parameters were specified
            return;

        for (const auto& [dumuxKey, istlKey] : dumuxToIstlSolverParams)
        {
            for (const auto& group : linearSolverGroups)
            {
                const auto fullDumuxKey = group + "." + dumuxKey;
                const auto value = getParam<std::string>(fullDumuxKey, "");
                if (!value.empty())
                {
                    params_[istlKey] = value;
                    break; // skip groups with smaller depth in the tree
                }
            }
        }

        // The type param is mandatory
        if (!params_.hasKey("type"))
            DUNE_THROW(Dune::InvalidStateException, "Solver factory needs a specified \"Type\" key to select the solver");
    }

    const std::string paramGroup_;
    std::unique_ptr<ParallelISTLHelper<GridView, AMGTraits>> parallelHelper_;
    bool firstCall_;
    Dune::InverseOperatorResult result_;
    Dune::ParameterTree params_;
};

template<class Matrix, class Vector, class Geometry>
std::vector<std::array<std::string, 2>>
IstlSolverFactoryBackend<Matrix, Vector, Geometry>::dumuxToIstlSolverParams =
{
    // solver params
    {"Verbosity", "verbose"},
    {"MaxIterations", "maxit"},
    {"ResidualReduction", "reduction"},
    {"Type", "type"},
    {"Restart", "restart"}, // cycles before restarting
    {"MaxOrthogonalizationVectors", "mmax"},

    // preconditioner params
    {"PreconditionerVerbosity", "preconditioner.verbosity"},
    {"PreconditionerType", "preconditioner.type"},
    {"PreconditionerIterations", "preconditioner.iterations"},
    {"PreconditionerRelaxation", "preconditioner.relaxation"},
    {"ILUOrder", "preconditioner.n"},
    {"ILUResort", "preconditioner.resort"},
    {"AmgSmootherRelaxation", "preconditioner.smootherRelaxation"},
    {"AmgSmootherIterations", "preconditioner.smootherIterations"},
    {"AmgMaxLevel", "preconditioner.maxLevel"},
    {"AmgCoarsenTarget", "preconditioner.coarsenTarget"},
    {"MinCoarseningRate", "preconditioner.minCoarseningRate"},
    {"AmgProlongationDampingFactor", "preconditioner.prolongationDampingFactor"},
    {"AmgAlpha", "preconditioner.alpha"},
    {"AmgBeta", "preconditioner.beta"},
    {"AmgAdditive", "preconditioner.additive"},
    {"AmgGamma", "preconditioner.gamma"},
    {"AmgPreSmoothingSteps", "preconditioner.preSteps"},
    {"AmgPostSmoothingSteps", "preconditioner.postSteps"},
    {"AmgCriterionSymmetric", "preconditioner.criterionSymmetric"},
    {"AmgStrengthMeasure", "preconditioner.strengthMeasure"},
    {"AmgDiagonalRowIndex", "preconditioner.diagonalRowIndex"},
    {"DefaultAggregationSizeMode", "preconditioner.defaultAggregationSizeMode"},
    {"defaultAggregationDimension", "preconditioner.defaultAggregationDimension"},
    {"MaxAggregateDistance", "preconditioner.maxAggregateDistance"},
    {"MinAggregateSize", "preconditioner.minAggregateSize"},
    {"MaxAggregateSize", "preconditioner.maxAggregateSize"}
};

} // end namespace Dumux

#else
#warning "Generic dune-istl solver factory backend needs dune-istl >= 2.7.1!"
#endif // DUNE version check
#endif // header guard
