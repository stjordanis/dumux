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
 * \brief Provides a parallel linear solver based on the ISTL AMG preconditioner
 *        and the ISTL BiCGSTAB solver.
 */
#ifndef DUMUX_PARALLEL_AMGBACKEND_HH
#define DUMUX_PARALLEL_AMGBACKEND_HH

#include <memory>

#include <dune/common/exceptions.hh>
#include <dune/common/parallel/indexset.hh>
#include <dune/common/parallel/mpicommunication.hh>
#include <dune/grid/common/capabilities.hh>
#include <dune/istl/paamg/amg.hh>
#include <dune/istl/paamg/pinfo.hh>
#include <dune/istl/solvers.hh>

#include <dumux/linear/solver.hh>
#include <dumux/linear/parallelhelpers.hh>
#include <dumux/linear/solvercategory.hh>

namespace Dumux {

namespace Detail {

template <class LinearSolverTraits>
class OldAMGBiCGSTABBackend : public LinearSolver
{
public:
    /*!
     * \brief Construct the backend for the sequential case only
     *
     * \param paramGroup the parameter group for parameter lookup
     */
    [[deprecated("Use new AMGBiCGSTABBackend<LinearSolverTraits, LinearAlgebraTraits> with 2nd template parameter.")]]
    OldAMGBiCGSTABBackend(const std::string& paramGroup = "")
    : LinearSolver(paramGroup)
    , isParallel_(Dune::MPIHelper::getCollectiveCommunication().size() > 1)
    {
        if (isParallel_)
            DUNE_THROW(Dune::InvalidStateException, "Using sequential constructor for parallel run. Use signature with gridView and dofMapper!");
    }

    /*!
     * \brief Construct the backend for parallel or sequential runs
     *
     * \param gridView the grid view on which we are performing the multi-grid
     * \param dofMapper an index mapper for dof entities
     * \param paramGroup the parameter group for parameter lookup
     */
    [[deprecated("Use new AMGBiCGSTABBackend<LinearSolverTraits, LinearAlgebraTraits> with 2nd template parameter.")]]
    OldAMGBiCGSTABBackend(const typename LinearSolverTraits::GridView& gridView,
                          const typename LinearSolverTraits::DofMapper& dofMapper,
                          const std::string& paramGroup = "")
    : LinearSolver(paramGroup)
#if HAVE_MPI
    , isParallel_(Dune::MPIHelper::getCollectiveCommunication().size() > 1)
#endif
    {
#if HAVE_MPI
        if constexpr (LinearSolverTraits::canCommunicate)
        {
            if (isParallel_)
                phelper_ = std::make_unique<ParallelISTLHelper<LinearSolverTraits>>(gridView, dofMapper);
        }
#endif
    }

    /*!
     * \brief Solve a linear system.
     *
     * \param A the matrix
     * \param x the seeked solution vector, containing the initial solution upon entry
     * \param b the right hand side vector
     */
    template<class Matrix, class Vector>
    bool solve(Matrix& A, Vector& x, Vector& b)
    {
#if HAVE_MPI
        solveSequentialOrParallel_(A, x, b);
#else
        solveSequential_(A, x, b);
#endif
        return result_.converged;
    }

    /*!
     * \brief The name of the solver
     */
    std::string name() const
    {
        return "AMG-preconditioned BiCGSTAB solver";
    }

    /*!
     * \brief The result containing the convergence history.
     */
    const Dune::InverseOperatorResult& result() const
    {
        return result_;
    }

    template<class Vector>
    Scalar norm(const Vector& x) const = delete;

private:

#if HAVE_MPI
    template<class Matrix, class Vector>
    void solveSequentialOrParallel_(Matrix& A, Vector& x, Vector& b)
    {
        if constexpr (LinearSolverTraits::canCommunicate)
        {
            if (isParallel_)
            {
                if (LinearSolverTraits::isNonOverlapping(phelper_->gridView()))
                {
                    using PTraits = typename LinearSolverTraits::template ParallelNonoverlapping<Matrix, Vector>;
                    solveParallel_<PTraits>(A, x, b);
                }
                else
                {
                    using PTraits = typename LinearSolverTraits::template ParallelOverlapping<Matrix, Vector>;
                    solveParallel_<PTraits>(A, x, b);
                }
            }
            else
                solveSequential_(A, x, b);
        }
        else
        {
            solveSequential_(A, x, b);
        }
    }

    template<class ParallelTraits, class Matrix, class Vector>
    void solveParallel_(Matrix& A, Vector& x, Vector& b)
    {
        using Comm = typename ParallelTraits::Comm;
        using LinearOperator = typename ParallelTraits::LinearOperator;
        using ScalarProduct = typename ParallelTraits::ScalarProduct;

        std::shared_ptr<Comm> comm;
        std::shared_ptr<LinearOperator> linearOperator;
        std::shared_ptr<ScalarProduct> scalarProduct;
        prepareLinearAlgebraParallel<LinearSolverTraits, ParallelTraits>(A, b, comm, linearOperator, scalarProduct, *phelper_);

        using SeqSmoother = Dune::SeqSSOR<Matrix, Vector, Vector>;
        using Smoother = typename ParallelTraits::template Preconditioner<SeqSmoother>;
        solveWithAmg_<Smoother>(A, x, b, linearOperator, comm, scalarProduct);
    }
#endif // HAVE_MPI

    template<class Matrix, class Vector>
    void solveSequential_(Matrix& A, Vector& x, Vector& b)
    {
        using Comm = Dune::Amg::SequentialInformation;
        using Traits = typename LinearSolverTraits::template Sequential<Matrix, Vector>;
        using LinearOperator = typename Traits::LinearOperator;
        using ScalarProduct = typename Traits::ScalarProduct;

        auto comm = std::make_shared<Comm>();
        auto linearOperator = std::make_shared<LinearOperator>(A);
        auto scalarProduct = std::make_shared<ScalarProduct>();

        using Smoother = Dune::SeqSSOR<Matrix, Vector, Vector>;
        solveWithAmg_<Smoother>(A, x, b, linearOperator, comm, scalarProduct);
    }

    template<class Smoother, class Matrix, class Vector, class LinearOperator, class Comm, class ScalarProduct>
    void solveWithAmg_(Matrix& A, Vector& x, Vector& b,
                       std::shared_ptr<LinearOperator>& linearOperator,
                       std::shared_ptr<Comm>& comm,
                       std::shared_ptr<ScalarProduct>& scalarProduct)
    {
        using SmootherArgs = typename Dune::Amg::SmootherTraits<Smoother>::Arguments;
        using Criterion = Dune::Amg::CoarsenCriterion<Dune::Amg::SymmetricCriterion<Matrix, Dune::Amg::FirstDiagonal>>;

        //! \todo Check whether the default accumulation mode atOnceAccu is needed.
        //! \todo make parameters changeable at runtime from input file / parameter tree
        Dune::Amg::Parameters params(15, 2000, 1.2, 1.6, Dune::Amg::atOnceAccu);
        params.setDefaultValuesIsotropic(LinearSolverTraits::GridView::dimension);
        params.setDebugLevel(this->verbosity());
        Criterion criterion(params);
        SmootherArgs smootherArgs;
        smootherArgs.iterations = 1;
        smootherArgs.relaxationFactor = 1;

        using Amg = Dune::Amg::AMG<LinearOperator, Vector, Smoother, Comm>;
        auto amg = std::make_shared<Amg>(*linearOperator, criterion, smootherArgs, *comm);

        Dune::BiCGSTABSolver<Vector> solver(*linearOperator, *scalarProduct, *amg, this->residReduction(), this->maxIter(),
                                            comm->communicator().rank() == 0 ? this->verbosity() : 0);

        solver.apply(x, b, result_);
    }

#if HAVE_MPI
    std::unique_ptr<ParallelISTLHelper<LinearSolverTraits>> phelper_;
#endif
    Dune::InverseOperatorResult result_;
    bool isParallel_ = false;
};

template <class LinearSolverTraits, class LinearAlgebraTraits>
class NewAMGBiCGSTABBackend : public LinearSolver
{
    using Matrix = typename LinearAlgebraTraits::Matrix;
    using Vector = typename LinearAlgebraTraits::Vector;
    using ScalarProduct = Dune::ScalarProduct<Vector>;
#if HAVE_MPI
    using Comm = Dune::OwnerOverlapCopyCommunication<Dune::bigunsignedint<96>, int>;
#endif
public:
    /*!
     * \brief Construct the backend for the sequential case only
     *
     * \param paramGroup the parameter group for parameter lookup
     */
    NewAMGBiCGSTABBackend(const std::string& paramGroup = "")
    : LinearSolver(paramGroup)
    , isParallel_(Dune::MPIHelper::getCollectiveCommunication().size() > 1)
    {
        if (isParallel_)
            DUNE_THROW(Dune::InvalidStateException, "Using sequential constructor for parallel run. Use signature with gridView and dofMapper!");

        solverCategory_ = Dune::SolverCategory::sequential;
        scalarProduct_ = std::make_shared<ScalarProduct>();
    }

    /*!
     * \brief Construct the backend for parallel or sequential runs
     *
     * \param gridView the grid view on which we are performing the multi-grid
     * \param dofMapper an index mapper for dof entities
     * \param paramGroup the parameter group for parameter lookup
     */
    NewAMGBiCGSTABBackend(const typename LinearSolverTraits::GridView& gridView,
                          const typename LinearSolverTraits::DofMapper& dofMapper,
                          const std::string& paramGroup = "")
    : LinearSolver(paramGroup)
#if HAVE_MPI
    , isParallel_(Dune::MPIHelper::getCollectiveCommunication().size() > 1)
#endif
    {
#if HAVE_MPI
        solverCategory_ = Detail::solverCategory<LinearSolverTraits>(gridView);
        if (solverCategory_ != Dune::SolverCategory::sequential)
        {
            phelper_ = std::make_unique<ParallelISTLHelper<LinearSolverTraits>>(gridView, dofMapper);
            comm_ = std::make_shared<Comm>(gridView.comm(), solverCategory_);
            scalarProduct_ = Dune::createScalarProduct<Vector>(*comm_, solverCategory_);
            phelper_->createParallelIndexSet(*comm_);
        }
        else
            scalarProduct_ = std::make_shared<ScalarProduct>();
#else
        solverCategory_ = Dune::SolverCategory::sequential;
#endif
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
#if HAVE_MPI
        solveSequentialOrParallel_(A, x, b);
#else
        solveSequential_(A, x, b);
#endif
        return result_.converged;
    }

    /*!
     * \brief The name of the solver
     */
    std::string name() const
    {
        return "AMG-preconditioned BiCGSTAB solver";
    }

    /*!
     * \brief The result containing the convergence history.
     */
    const Dune::InverseOperatorResult& result() const
    {
        return result_;
    }

    Scalar norm(const Vector& x) const
    {
#if HAVE_MPI
        if constexpr (LinearSolverTraits::canCommunicate)
        {
            if (solverCategory_ == Dune::SolverCategory::nonoverlapping)
            {
                auto y(x); // make a copy because the vector needs to be made consistent
                using GV = typename LinearSolverTraits::GridView;
                using DM = typename LinearSolverTraits::DofMapper;
                ParallelVectorHelper<GV, DM, LinearSolverTraits::dofCodim> vectorHelper(phelper_->gridView(), phelper_->dofMapper());
                vectorHelper.makeNonOverlappingConsistent(y);
                return scalarProduct_->norm(y);
            }
        }
#endif
        return scalarProduct_->norm(x);
    }

private:

#if HAVE_MPI
    void solveSequentialOrParallel_(Matrix& A, Vector& x, Vector& b)
    {
        if constexpr (LinearSolverTraits::canCommunicate)
        {
            if (isParallel_)
            {
                if (LinearSolverTraits::isNonOverlapping(phelper_->gridView()))
                {
                    using PTraits = typename LinearSolverTraits::template ParallelNonoverlapping<Matrix, Vector>;
                    solveParallel_<PTraits>(A, x, b);
                }
                else
                {
                    using PTraits = typename LinearSolverTraits::template ParallelOverlapping<Matrix, Vector>;
                    solveParallel_<PTraits>(A, x, b);
                }
            }
            else
                solveSequential_(A, x, b);
        }
        else
        {
            solveSequential_(A, x, b);
        }
    }

    template<class ParallelTraits>
    void solveParallel_(Matrix& A, Vector& x, Vector& b)
    {
        prepareLinearAlgebraParallel<LinearSolverTraits, ParallelTraits>(A, b, *phelper_);
        auto linearOperator = std::make_shared<typename ParallelTraits::LinearOperator>(A, *comm_);

        using SeqSmoother = Dune::SeqSSOR<Matrix, Vector, Vector>;
        using Smoother = typename ParallelTraits::template Preconditioner<SeqSmoother>;
        solveWithAmg_<Smoother>(A, x, b, linearOperator, comm_);
    }
#endif // HAVE_MPI

    void solveSequential_(Matrix& A, Vector& x, Vector& b)
    {
        using SequentialTraits = typename LinearSolverTraits::template Sequential<Matrix, Vector>;
        auto linearOperator = std::make_shared<typename SequentialTraits::LinearOperator>(A);

        auto comm = std::make_shared<Dune::Amg::SequentialInformation>();

        using Smoother = Dune::SeqSSOR<Matrix, Vector, Vector>;
        solveWithAmg_<Smoother>(A, x, b, linearOperator, comm);
    }

    template<class Smoother, class LinearOperator, class Comm>
    void solveWithAmg_(Matrix& A, Vector& x, Vector& b,
                       std::shared_ptr<LinearOperator>& linearOperator,
                       std::shared_ptr<Comm>& comm)
    {
        using SmootherArgs = typename Dune::Amg::SmootherTraits<Smoother>::Arguments;
        using Criterion = Dune::Amg::CoarsenCriterion<Dune::Amg::SymmetricCriterion<Matrix, Dune::Amg::FirstDiagonal>>;

        //! \todo Check whether the default accumulation mode atOnceAccu is needed.
        //! \todo make parameters changeable at runtime from input file / parameter tree
        Dune::Amg::Parameters params(15, 2000, 1.2, 1.6, Dune::Amg::atOnceAccu);
        params.setDefaultValuesIsotropic(LinearSolverTraits::GridView::dimension);
        params.setDebugLevel(this->verbosity());
        Criterion criterion(params);
        SmootherArgs smootherArgs;
        smootherArgs.iterations = 1;
        smootherArgs.relaxationFactor = 1;

        using Amg = Dune::Amg::AMG<LinearOperator, Vector, Smoother, Comm>;
        auto amg = std::make_shared<Amg>(*linearOperator, criterion, smootherArgs, *comm);

        Dune::BiCGSTABSolver<Vector> solver(*linearOperator, *scalarProduct_, *amg, this->residReduction(), this->maxIter(),
                                            comm->communicator().rank() == 0 ? this->verbosity() : 0);

        solver.apply(x, b, result_);
    }

#if HAVE_MPI
    std::unique_ptr<ParallelISTLHelper<LinearSolverTraits>> phelper_;
    std::shared_ptr<Comm> comm_;
#endif
    Dune::SolverCategory::Category solverCategory_;
    std::shared_ptr<ScalarProduct> scalarProduct_;
    Dune::InverseOperatorResult result_;
    bool isParallel_ = false;
};

template<int i>
struct AMGImplHelper
{
    template<class ...T>
     using type = NewAMGBiCGSTABBackend<T...>;
};

template<>
struct AMGImplHelper<1>
{
    template<class T>
    using type = OldAMGBiCGSTABBackend<T>;
};

} // end namespace Detail

/*!
 * \ingroup Linear
 * \brief A linear solver based on the ISTL AMG preconditioner
 *        and the ISTL BiCGSTAB solver.
 */
template<class ...T>
using AMGBiCGSTABBackend = typename Detail::AMGImplHelper<sizeof...(T)>::template type<T...>;

} // end namespace Dumux

#endif
