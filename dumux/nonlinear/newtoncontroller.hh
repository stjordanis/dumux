// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
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
 * \brief Reference implementation of a controller class for the Newton solver.
 *
 * Usually this controller should be sufficient.
 */
#ifndef DUMUX_NEWTON_CONTROLLER_HH
#define DUMUX_NEWTON_CONTROLLER_HH

#include <dumux/common/propertysystem.hh>
#include <dumux/common/exceptions.hh>
#include <dumux/common/math.hh>
#include <dumux/io/vtkmultiwriter.hh>
#include <dumux/linear/boxlinearsolver.hh>

#include "newtonconvergencewriter.hh"

namespace Dumux
{
template <class TypeTag>
class NewtonController;

template <class TypeTag>
class NewtonConvergenceWriter;

namespace Properties
{
//! Specifies the implementation of the Newton controller
NEW_PROP_TAG(NewtonController);

//! Specifies the type of the actual Newton method
NEW_PROP_TAG(NewtonMethod);

//! Specifies the type of a solution
NEW_PROP_TAG(SolutionVector);

//! Specifies the type of a global Jacobian matrix
NEW_PROP_TAG(JacobianMatrix);

//! Specifies the type of the Vertex mapper
NEW_PROP_TAG(VertexMapper);

//! specifies the type of the time manager
NEW_PROP_TAG(TimeManager);

//! specifies whether the convergence rate and the global residual
//! gets written out to disk for every Newton iteration (default is false)
NEW_PROP_TAG(NewtonWriteConvergence);

//! Specifies whether the Jacobian matrix should only be reassembled
//! if the current solution deviates too much from the evaluation point
NEW_PROP_TAG(ImplicitEnablePartialReassemble);

/*!
 * \brief Specifies whether the update should be done using the line search
 *        method instead of the plain Newton method.
 *
 * Whether this property has any effect depends on whether the line
 * search method is implemented for the actual model's Newton
 * controller's update() method. By default line search is not used.
 */
NEW_PROP_TAG(NewtonUseLineSearch);

//! indicate whether the shift criterion should be used
NEW_PROP_TAG(NewtonEnableShiftCriterion);
NEW_PROP_TAG(NewtonEnableRelativeCriterion);// DEPRECATED

//! the value for the maximum relative shift below which convergence is declared
NEW_PROP_TAG(NewtonMaxRelativeShift);
NEW_PROP_TAG(NewtonRelTolerance);// DEPRECATED

//! indicate whether the residual criterion should be used
NEW_PROP_TAG(NewtonEnableResidualCriterion);
NEW_PROP_TAG(NewtonEnableAbsoluteCriterion);// DEPRECATED

//! the value for the residual reduction below which convergence is declared
NEW_PROP_TAG(NewtonResidualReduction);
NEW_PROP_TAG(NewtonAbsTolerance);// DEPRECATED

//! indicate whether both of the criteria should be satisfied to declare convergence
NEW_PROP_TAG(NewtonSatisfyResidualAndShiftCriterion);
NEW_PROP_TAG(NewtonSatisfyAbsAndRel);// DEPRECATED

/*!
 * \brief The number of iterations at which the Newton method
 *        should aim at.
 *
 * This is used to control the time-step size. The heuristic used
 * is to scale the last time-step size by the deviation of the
 * number of iterations used from the target steps.
 */
NEW_PROP_TAG(NewtonTargetSteps);

//! Number of maximum iterations for the Newton method.
NEW_PROP_TAG(NewtonMaxSteps);

//! The assembler for the Jacobian matrix
NEW_PROP_TAG(JacobianAssembler);

// set default values
SET_TYPE_PROP(NewtonMethod, NewtonController, Dumux::NewtonController<TypeTag>);
SET_BOOL_PROP(NewtonMethod, NewtonWriteConvergence, false);
SET_BOOL_PROP(NewtonMethod, NewtonUseLineSearch, false);

// Renaming: EnableRelativeCriterion -> EnableShiftCriterion
SET_BOOL_PROP(NewtonMethod, NewtonEnableShiftCriterion,
              GET_PROP_VALUE(TypeTag, NewtonEnableRelativeCriterion));
SET_BOOL_PROP(NewtonMethod, NewtonEnableRelativeCriterion, true);// DEPRECATED

// Renaming: EnableAbsoluteCriterion -> EnableResidualCriterion
SET_BOOL_PROP(NewtonMethod, NewtonEnableResidualCriterion,
              GET_PROP_VALUE(TypeTag, NewtonEnableAbsoluteCriterion));
SET_BOOL_PROP(NewtonMethod, NewtonEnableAbsoluteCriterion, false);// DEPRECATED

// Renaming: SatisfyAbsAndRel -> SatisfyResidualAndShiftCriterion
SET_BOOL_PROP(NewtonMethod, NewtonSatisfyResidualAndShiftCriterion,
              GET_PROP_VALUE(TypeTag, NewtonSatisfyAbsAndRel));
SET_BOOL_PROP(NewtonMethod, NewtonSatisfyAbsAndRel, false);// DEPRECATED

// Renaming: RelTolerance -> MaxRelativeShift
SET_SCALAR_PROP(NewtonMethod, NewtonMaxRelativeShift,
              GET_PROP_VALUE(TypeTag, NewtonRelTolerance));
SET_SCALAR_PROP(NewtonMethod, NewtonRelTolerance, 1e-8);// DEPRECATED

// Renaming: AbsTolerance -> ResidualReduction
SET_SCALAR_PROP(NewtonMethod, NewtonResidualReduction,
              GET_PROP_VALUE(TypeTag, NewtonAbsTolerance));
SET_SCALAR_PROP(NewtonMethod, NewtonAbsTolerance, 1e-5);// DEPRECATED

SET_INT_PROP(NewtonMethod, NewtonTargetSteps, 10);
SET_INT_PROP(NewtonMethod, NewtonMaxSteps, 18);

} // end namespace Properties

/*!
 * \ingroup Newton
 * \brief A reference implementation of a Newton controller specific
 *        for the box scheme.
 *
 * If you want to specialize only some methods but are happy with the
 * defaults of the reference controller, derive your controller from
 * this class and simply overload the required methods.
 */
template <class TypeTag>
class NewtonController
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, NewtonController) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

    typedef typename GET_PROP_TYPE(TypeTag, Problem) Problem;
    typedef typename GET_PROP_TYPE(TypeTag, Model) Model;
    typedef typename GET_PROP_TYPE(TypeTag, NewtonMethod) NewtonMethod;
    typedef typename GET_PROP_TYPE(TypeTag, JacobianMatrix) JacobianMatrix;
    typedef typename GET_PROP_TYPE(TypeTag, JacobianAssembler) JacobianAssembler;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;
    typedef typename GET_PROP_TYPE(TypeTag, VertexMapper) VertexMapper;

    typedef typename GET_PROP_TYPE(TypeTag, SolutionVector) SolutionVector;

    typedef NewtonConvergenceWriter<TypeTag> ConvergenceWriter;

    typedef typename GET_PROP_TYPE(TypeTag, LinearSolver) LinearSolver;

public:
    /*!
     * \brief
     */
// Avoid warnings when deprecated member variables are default-initialized
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    NewtonController(const Problem &problem)
        : endIterMsgStream_(std::ostringstream::out)
        , convergenceWriter_(asImp_())
        , linearSolver_(problem)
    {
        enablePartialReassemble_ = GET_PARAM_FROM_GROUP(TypeTag, bool, Implicit, EnablePartialReassemble);
        enableJacobianRecycling_ = GET_PARAM_FROM_GROUP(TypeTag, bool, Implicit, EnableJacobianRecycling);

        useLineSearch_ = GET_PARAM_FROM_GROUP(TypeTag, bool, Newton, UseLineSearch);
        enableShiftCriterion_ = GET_PARAM_FROM_GROUP(TypeTag, bool, Newton, EnableShiftCriterion);
        enableResidualCriterion_ = GET_PARAM_FROM_GROUP(TypeTag, bool, Newton, EnableResidualCriterion);
        satisfyResidualAndShiftCriterion_ = GET_PARAM_FROM_GROUP(TypeTag, bool, Newton, SatisfyResidualAndShiftCriterion);
        if (!enableShiftCriterion_ && !enableResidualCriterion_)
        {
            DUNE_THROW(Dune::NotImplemented, "at least one of NewtonEnableShiftCriterion or "
                    << "NewtonEnableResidualCriterion has to be set to true");
        }

        setMaxRelativeShift(GET_PARAM_FROM_GROUP(TypeTag, Scalar, Newton, MaxRelativeShift));
        setResidualReduction(GET_PARAM_FROM_GROUP(TypeTag, Scalar, Newton, ResidualReduction));
        setTargetSteps(GET_PARAM_FROM_GROUP(TypeTag, int, Newton, TargetSteps));
        setMaxSteps(GET_PARAM_FROM_GROUP(TypeTag, int, Newton, MaxSteps));

        verbose_ = true;
        numSteps_ = 0;
    }
#pragma GCC diagnostic pop

    /*!
     * \brief Destructor
     */
    ~NewtonController()
    {
        typedef typename GET_PROP(TypeTag, ParameterTree) Params;
        const Dune::ParameterTree &tree = Params::tree();

        // check if deprecated parameter names have been set run-time:
        if (tree.hasKey("Newton.EnableRelativeCriterion")
            || tree.hasKey("Newton.RelTolerance")
            || tree.hasKey("Newton.EnableAbsoluteCriterion")
            || tree.hasKey("Newton.AbsTolerance")
            || tree.hasKey("Newton.SatisfyAbsAndRel"))
        {
            std::cout << std::endl << "[Newton] The following DEPRECATED parameters"
                      << "are set run-time and are therefore not used:" << std::endl;

            if (tree.hasKey("Newton.EnableRelativeCriterion"))
            {
                std::cout << "EnableRelativeCriterion: use EnableShiftCriterion instead" << std::endl;
            }
            if (tree.hasKey("Newton.RelTolerance"))
            {
                std::cout << "RelTolerance: use MaxRelativeShift instead" << std::endl;
            }
            if (tree.hasKey("Newton.EnableAbsoluteCriterion"))
            {
                std::cout << "EnableAbsoluteCriterion: use EnableResidualCriterion instead" << std::endl;
            }
            if (tree.hasKey("Newton.AbsTolerance"))
            {
                std::cout << "AbsTolerance: use ResidualReduction instead" << std::endl;
            }
            if (tree.hasKey("Newton.SatisfyAbsAndRel"))
            {
                std::cout << "SatisfyAbsAndRel: use SatisfyResidualAndShiftCriterion instead" << std::endl;
            }
        }

        // check if deprecated property names have been set compile-time to
        // a non-default value:
        if (GET_PROP_VALUE(TypeTag, NewtonEnableRelativeCriterion) != true
            || GET_PROP_VALUE(TypeTag, NewtonRelTolerance) != 1e-8
            || GET_PROP_VALUE(TypeTag, NewtonEnableAbsoluteCriterion) != false
            || GET_PROP_VALUE(TypeTag, NewtonAbsTolerance) != 1e-5
            || GET_PROP_VALUE(TypeTag, NewtonSatisfyAbsAndRel) != false)
        {
            std::cout << std::endl << "[Newton] The following DEPRECATED properties"
                      << " are set compile-time and the corresponding new properties are used:"
                      << std::endl;

            if (GET_PROP_VALUE(TypeTag, NewtonEnableRelativeCriterion) != true)
            {
                std::cout << "NewtonEnableRelativeCriterion: use NewtonEnableShiftCriterion instead" << std::endl;
            }
            if (GET_PROP_VALUE(TypeTag, NewtonRelTolerance) != 1e-8)
            {
                std::cout << "NewtonRelTolerance: use NewtonMaxRelativeShift instead" << std::endl;
            }
            if (GET_PROP_VALUE(TypeTag, NewtonEnableAbsoluteCriterion) != false)
            {
                std::cout << "NewtonEnableAbsoluteCriterion: use NewtonEnableResidualCriterion instead" << std::endl;
            }
            if (GET_PROP_VALUE(TypeTag, NewtonAbsTolerance) != 1e-5)
            {
                std::cout << "NewtonAbsTolerance: use NewtonResidualReduction instead" << std::endl;
            }
            if (GET_PROP_VALUE(TypeTag, NewtonSatisfyAbsAndRel) != false)
            {
                std::cout << "NewtonSatisfyAbsAndRel: use NewtonSatisfyResidualAndShiftCriterion instead" << std::endl;
            }
        }
    }

    /*!
     * \brief Set the maximum acceptable difference of any primary variable
     * between two iterations for declaring convergence.
     *
     * \param tolerance The maximum relative shift between two Newton
     *                  iterations at which the scheme is considered finished
     */
    void setMaxRelativeShift(Scalar tolerance)
    { shiftTolerance_ = tolerance; }

    void setRelTolerance(Scalar tolerance)
    DUNE_DEPRECATED_MSG("use setMaxRelativeShift instead")
    { setMaxRelativeShift(tolerance); }

    /*!
     * \brief Set the maximum acceptable residual norm reduction.
     *
     * \param tolerance The maximum reduction of the residual norm
     *                  at which the scheme is considered finished
     */
    void setResidualReduction(Scalar tolerance)
    { reductionTolerance_ = tolerance; }

    void setAbsTolerance(Scalar tolerance)
    DUNE_DEPRECATED_MSG("use setResidualReduction instead")
    { setResidualReduction(tolerance); }

    /*!
     * \brief Set the number of iterations at which the Newton method
     *        should aim at.
     *
     * This is used to control the time-step size. The heuristic used
     * is to scale the last time-step size by the deviation of the
     * number of iterations used from the target steps.
     *
     * \param targetSteps Number of iterations which are considered "optimal"
     */
    void setTargetSteps(int targetSteps)
    { targetSteps_ = targetSteps; }

    /*!
     * \brief Set the number of iterations after which the Newton
     *        method gives up.
     *
     * \param maxSteps Number of iterations after we give up
     */
    void setMaxSteps(int maxSteps)
    { maxSteps_ = maxSteps; }

    /*!
     * \brief Returns true if another iteration should be done.
     *
     * \param uCurrentIter The solution of the current Newton iteration
     */
    bool newtonProceed(const SolutionVector &uCurrentIter)
    {
        if (numSteps_ < 2)
            return true; // we always do at least two iterations
        else if (asImp_().newtonConverged()) {
            return false; // we are below the desired tolerance
        }
        else if (numSteps_ >= maxSteps_) {
            // We have exceeded the allowed number of steps. If the
            // maximum relative shift was reduced by a factor of at least 4,
            // we proceed even if we are above the maximum number of steps.
            if (enableShiftCriterion_)
                return shift_*4.0 < lastShift_;
            else
                return reduction_*4.0 < lastReduction_;
        }

        return true;
    }

    /*!
     * \brief Returns true if the error of the solution is below the
     *        tolerance.
     */
    bool newtonConverged() const
    {
        if (enableShiftCriterion_ && !enableResidualCriterion_)
        {
            return shift_ <= shiftTolerance_;
        }
        else if (!enableShiftCriterion_ && enableResidualCriterion_)
        {
            return reduction_ <= reductionTolerance_;
        }
        else if (satisfyResidualAndShiftCriterion_)
        {
            return shift_ <= shiftTolerance_
                    && reduction_ <= reductionTolerance_;
        }
        else
        {
            return shift_ <= shiftTolerance_
                    || reduction_ <= reductionTolerance_;
        }

        return false;
    }

    /*!
     * \brief Called before the Newton method is applied to an
     *        non-linear system of equations.
     *
     * \param method The object where the NewtonMethod is executed
     * \param u The initial solution
     */
    void newtonBegin(NewtonMethod &method, const SolutionVector &u)
    {
        method_ = &method;
        numSteps_ = 0;

        if (GET_PARAM_FROM_GROUP(TypeTag, bool, Newton, WriteConvergence))
            convergenceWriter_.beginTimestep();
    }

    /*!
     * \brief Indicates the beginning of a Newton iteration.
     */
    void newtonBeginStep()
    {
        lastShift_ = shift_;
        lastReduction_ = reduction_;
    }

    /*!
     * \brief Returns the number of steps done since newtonBegin() was
     *        called.
     */
    int newtonNumSteps()
    { return numSteps_; }

    /*!
     * \brief Update the maximum relative shift of the solution compared to
     *        the previous iteration.
     *
     * \param uLastIter The current iterative solution
     * \param deltaU The difference between the current and the next solution
     */
    void newtonUpdateShift(const SolutionVector &uLastIter,
                           const SolutionVector &deltaU)
    {
        shift_ = 0;

        for (int i = 0; i < int(uLastIter.size()); ++i) {
            typename SolutionVector::block_type uNewI = uLastIter[i];
            uNewI -= deltaU[i];

            Scalar shiftAtDof = model_().relativeShiftAtDof(uLastIter[i],
                                                            uNewI);
            shift_ = std::max(shift_, shiftAtDof);
        }

        if (gridView_().comm().size() > 1)
            shift_ = gridView_().comm().max(shift_);
    }

    void newtonUpdateRelError(const SolutionVector &uLastIter,
                              const SolutionVector &deltaU)
    DUNE_DEPRECATED_MSG("use newtonUpdateShift instead")
    { newtonUpdateShift(uLastIter, deltaU); }

    /*!
     * \brief Solve the linear system of equations \f$\mathbf{A}x - b = 0\f$.
     *
     * Throws Dumux::NumericalProblem if the linear solver didn't
     * converge.
     *
     * \param A The matrix of the linear system of equations
     * \param x The vector which solves the linear system
     * \param b The right hand side of the linear system
     */
    void newtonSolveLinear(JacobianMatrix &A,
                           SolutionVector &x,
                           SolutionVector &b)
    {
        try {
            if (numSteps_ == 0)
            {
                Scalar norm2 = b.two_norm2();
                if (gridView_().comm().size() > 1)
                    norm2 = gridView_().comm().sum(norm2);

                initialResidual_ = std::sqrt(norm2);
                lastReduction_ = initialResidual_;
            }

            int converged = linearSolver_.solve(A, x, b);

            // make sure all processes converged
            int convergedRemote = converged;
            if (gridView_().comm().size() > 1)
                convergedRemote = gridView_().comm().min(converged);

            if (!converged) {
                DUNE_THROW(NumericalProblem,
                           "Linear solver did not converge");
            }
            else if (!convergedRemote) {
                DUNE_THROW(NumericalProblem,
                           "Linear solver did not converge on a remote process");
            }
        }
        catch (Dune::MatrixBlockError e) {
            // make sure all processes converged
            int converged = 0;
            if (gridView_().comm().size() > 1)
                converged = gridView_().comm().min(converged);

            Dumux::NumericalProblem p;
            std::string msg;
            std::ostringstream ms(msg);
            ms << e.what() << "M=" << A[e.r][e.c];
            p.message(ms.str());
            throw p;
        }
        catch (const Dune::Exception &e) {
            // make sure all processes converged
            int converged = 0;
            if (gridView_().comm().size() > 1)
                converged = gridView_().comm().min(converged);

            Dumux::NumericalProblem p;
            p.message(e.what());
            throw p;
        }
    }

    /*!
     * \brief Update the current solution with a delta vector.
     *
     * The error estimates required for the newtonConverged() and
     * newtonProceed() methods should be updated inside this method.
     *
     * Different update strategies, such as line search and chopped
     * updates can be implemented. The default behavior is just to
     * subtract deltaU from uLastIter, i.e.
     * \f[ u^{k+1} = u^k - \Delta u^k \f]
     *
     * \param uCurrentIter The solution vector after the current iteration
     * \param uLastIter The solution vector after the last iteration
     * \param deltaU The delta as calculated from solving the linear
     *               system of equations. This parameter also stores
     *               the updated solution.
     */
    void newtonUpdate(SolutionVector &uCurrentIter,
                      const SolutionVector &uLastIter,
                      const SolutionVector &deltaU)
    {
        if (enableShiftCriterion_ || enablePartialReassemble_)
            newtonUpdateShift(uLastIter, deltaU);

        // compute the vertex and element colors for partial reassembly
        if (enablePartialReassemble_) {
            const Scalar minReasmTol = 1e-2*shiftTolerance_;
            const Scalar maxReasmTol = 1e1*shiftTolerance_;
            Scalar reassembleTol = std::max(minReasmTol, std::min(maxReasmTol, this->shift_/1e4));
            //Scalar reassembleTol = minReasmTol;

            this->model_().jacobianAssembler().updateDiscrepancy(uLastIter, deltaU);
            this->model_().jacobianAssembler().computeColors(reassembleTol);
        }

        writeConvergence_(uLastIter, deltaU);

        if (useLineSearch_)
        {
            lineSearchUpdate_(uCurrentIter, uLastIter, deltaU);
        }
        else {
            for (unsigned int i = 0; i < uLastIter.size(); ++i) {
                uCurrentIter[i] = uLastIter[i];
                uCurrentIter[i] -= deltaU[i];
            }

            if (enableResidualCriterion_)
            {
                SolutionVector tmp(uLastIter);
                reduction_ = this->method().model().globalResidual(tmp, uCurrentIter);
                reduction_ /= initialResidual_;
            }
        }

    }

    /*!
     * \brief Indicates that one Newton iteration was finished.
     *
     * \param uCurrentIter The solution after the current Newton iteration
     * \param uLastIter The solution at the beginning of the current Newton iteration
     */
    void newtonEndStep(const SolutionVector &uCurrentIter,
                       const SolutionVector &uLastIter)
    {
        ++numSteps_;

        if (verbose())
        {
            std::cout << "\rNewton iteration " << numSteps_ << " done";
            if (enableShiftCriterion_)
                std::cout << ", maximum relative shift = " << shift_;
            if (enableResidualCriterion_)
                std::cout << ", residual reduction = " << reduction_;
            std::cout << endIterMsg().str() << "\n";
        }
        endIterMsgStream_.str("");

//         When the newton iterations is done: ask the model to check whether it makes sense.
        model_().checkPlausibility() ;
    }

    /*!
     * \brief Indicates that we're done solving the non-linear system
     *        of equations.
     */
    void newtonEnd()
    {
        if (GET_PARAM_FROM_GROUP(TypeTag, bool, Newton, WriteConvergence))
            convergenceWriter_.endTimestep();
    }

    /*!
     * \brief Called if the Newton method broke down.
     *
     * This method is called _after_ newtonEnd()
     */
    void newtonFail()
    {
        model_().jacobianAssembler().reassembleAll();
        numSteps_ = targetSteps_*2;
    }

    /*!
     * \brief Called when the Newton method was successful.
     *
     * This method is called _after_ newtonEnd()
     */
    void newtonSucceed()
    {
        if (enableJacobianRecycling_)
            model_().jacobianAssembler().setMatrixReuseable(true);
        else
            model_().jacobianAssembler().reassembleAll();
    }

    /*!
     * \brief Suggest a new time-step size based on the old time-step
     *        size.
     *
     * The default behavior is to suggest the old time-step size
     * scaled by the ratio between the target iterations and the
     * iterations required to actually solve the last time-step.
     */
    Scalar suggestTimeStepSize(Scalar oldTimeStep) const
    {
        // be aggressive reducing the time-step size but
        // conservative when increasing it. the rationale is
        // that we want to avoid failing in the next Newton
        // iteration which would require another linearization
        // of the problem.
        if (numSteps_ > targetSteps_) {
            Scalar percent = Scalar(numSteps_ - targetSteps_)/targetSteps_;
            return oldTimeStep/(1.0 + percent);
        }
        
        Scalar percent = Scalar(targetSteps_ - numSteps_)/targetSteps_;
        return oldTimeStep*(1.0 + percent/1.2);
    }

    /*!
     * \brief Returns a reference to the current Newton method
     *        which is controlled by this controller.
     */
    NewtonMethod &method()
    { return *method_; }

    /*!
     * \brief Returns a reference to the current Newton method
     *        which is controlled by this controller.
     */
    const NewtonMethod &method() const
    { return *method_; }

    std::ostringstream &endIterMsg()
    { return endIterMsgStream_; }

    /*!
     * \brief Specifies if the Newton method ought to be chatty.
     */
    void setVerbose(bool val)
    { verbose_ = val; }

    /*!
     * \brief Returns true if the Newton method ought to be chatty.
     */
    bool verbose() const
    { return verbose_ && gridView_().comm().rank() == 0; }

protected:
    /*!
     * \brief Returns a reference to the grid view.
     */
    const GridView &gridView_() const
    { return problem_().gridView(); }

    /*!
     * \brief Returns a reference to the vertex mapper.
     */
    const VertexMapper &vertexMapper_() const
    { return model_().vertexMapper(); }

    /*!
     * \brief Returns a reference to the problem.
     */
    Problem &problem_()
    { return method_->problem(); }

    /*!
     * \brief Returns a reference to the problem.
     */
    const Problem &problem_() const
    { return method_->problem(); }

    /*!
     * \brief Returns a reference to the time manager.
     */
    TimeManager &timeManager_()
    { return problem_().timeManager(); }

    /*!
     * \brief Returns a reference to the time manager.
     */
    const TimeManager &timeManager_() const
    { return problem_().timeManager(); }

    /*!
     * \brief Returns a reference to the problem.
     */
    Model &model_()
    { return problem_().model(); }

    /*!
     * \brief Returns a reference to the problem.
     */
    const Model &model_() const
    { return problem_().model(); }

    // returns the actual implementation for the controller we do
    // it this way in order to allow "poor man's virtual methods",
    // i.e. methods of subclasses which can be called by the base
    // class.
    Implementation &asImp_()
    { return *static_cast<Implementation*>(this); }
    const Implementation &asImp_() const
    { return *static_cast<const Implementation*>(this); }

    void writeConvergence_(const SolutionVector &uLastIter,
                           const SolutionVector &deltaU)
    {
        if (GET_PARAM_FROM_GROUP(TypeTag, bool, Newton, WriteConvergence)) {
            convergenceWriter_.beginIteration(this->gridView_());
            convergenceWriter_.writeFields(uLastIter, deltaU);
            convergenceWriter_.endIteration();
        }
    }

    void lineSearchUpdate_(SolutionVector &uCurrentIter,
                           const SolutionVector &uLastIter,
                           const SolutionVector &deltaU)
    {
       Scalar lambda = 1.0;
       SolutionVector tmp(uLastIter);

       while (true) {
           uCurrentIter = deltaU;
           uCurrentIter *= -lambda;
           uCurrentIter += uLastIter;

           // calculate the residual of the current solution
           reduction_ = this->method().model().globalResidual(tmp, uCurrentIter);
           reduction_ /= initialResidual_;

           if (reduction_ < lastReduction_ || lambda <= 0.125) {
               this->endIterMsg() << ", residual reduction " << lastReduction_ << "->"  << reduction_ << "@lambda=" << lambda;
               return;
           }

           // try with a smaller update
           lambda /= 2.0;
       }
    }

    std::ostringstream endIterMsgStream_;

    NewtonMethod *method_;
    bool verbose_;

    ConvergenceWriter convergenceWriter_;

    // shift criterion variables
    Scalar shift_;
    Scalar lastShift_;
    Scalar shiftTolerance_;

    // residual criterion variables
    Scalar reduction_;
    Scalar lastReduction_;
    Scalar initialResidual_;
    Scalar reductionTolerance_;

    // optimal number of iterations we want to achieve
    int targetSteps_;
    // maximum number of iterations we do before giving up
    int maxSteps_;
    // actual number of steps done so far
    int numSteps_;

    // the linear solver
    LinearSolver linearSolver_;

    bool enablePartialReassemble_;
    bool enableJacobianRecycling_;
    bool useLineSearch_;
    bool enableShiftCriterion_;
    bool enableResidualCriterion_;
    bool satisfyResidualAndShiftCriterion_;

    // deprecated member variables
    Scalar error_ DUNE_DEPRECATED_MSG("use shift_ instead");
    Scalar lastError_ DUNE_DEPRECATED_MSG("use lastShift_ instead");
    Scalar tolerance_ DUNE_DEPRECATED_MSG("use shiftTolerance_ instead");
    Scalar absoluteError_ DUNE_DEPRECATED_MSG("use reduction_ instead");
    Scalar lastAbsoluteError_ DUNE_DEPRECATED_MSG("use lastReduction_ instead");
    Scalar initialAbsoluteError_ DUNE_DEPRECATED_MSG("use initialResidual_ instead");
    Scalar absoluteTolerance_ DUNE_DEPRECATED_MSG("use reductionTolerance_ instead");
    bool enableRelativeCriterion_ DUNE_DEPRECATED_MSG("use enableShiftCriterion_ instead");
    bool enableAbsoluteCriterion_ DUNE_DEPRECATED_MSG("use enableResidualCriterion_ instead");
    bool satisfyAbsAndRel_ DUNE_DEPRECATED_MSG("use satisfyResidualAndShiftCriterion_ instead");
};
} // namespace Dumux

#endif
