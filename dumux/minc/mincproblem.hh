

#ifndef DUNE_MINCPROBLEM_HH
#define DUNE_MINCPROBLEM_HH

#include<iostream>
#include<iomanip>

#include<dune/common/fvector.hh>
#include<dune/common/fmatrix.hh>
#include<dune/istl/bvector.hh>
#include<dune/common/exceptions.hh>
#include<dune/grid/common/grid.hh>
#include<dune/grid/common/referenceelements.hh>
#include<dune/grid/utility/intersectiongetter.hh>
#include<dune/disc/operators/boundaryconditions.hh>
#include<dumux/material/twophaserelations.hh>
#include<test/minc/minc_soilproperties.hh>


/**
 * @file
 * @brief  Base class for defining an instance of the Minc problem
 * @author Bernd Flemisch
 */

namespace Dune {
//! base class that defines the parameters of a diffusion equation
/*! An interface for defining parameters for the stationary diffusion equation
 * \f$ - \text{div}\, (\lambda K \text{grad}\, p ) = q, \f$,
 * \f$p = g\f$ on \f$\Gamma_1\f$, and \f$\lambda K \text{grad}\, p = J\f$
 * on \f$\Gamma_2\f$. Here,
 * \f$p\f$ denotes the pressure, \f$K\f$ the absolute permeability,
 * and \f$\lambda\f$ the total mobility, possibly depending on the
 * saturation.
 *
 *    Template parameters are:
 *
 *    - Grid  a DUNE grid type
 *    - Scalar    type used for return values
 */
template<class Grid, class Scalar, int numEq> class MincProblem {
    typedef typename Grid::ctype DT;
    enum {dim=Grid::dimension};
    typedef typename Grid::Traits::template Codim<0>::Entity Element;
    typedef typename IntersectionIteratorGetter<Grid,LeafTag>::IntersectionIterator
    IntersectionIterator;
public:
    //! evaluate source term
    /*! evaluate source term at given location
      @param[in]  x    position in global coordinates
      @param[in]  e    entity of codim 0
      @param[in]  xi   position in reference element of e
      \return     value of source term
    */
    virtual FieldVector<Scalar,numEq> q(const FieldVector<Scalar,dim>& globalPos, const Element& element,
                                        const FieldVector<Scalar,dim>& localPos) const = 0;

    //! return type of boundary condition at the given global coordinate
    /*! return type of boundary condition at the given global coordinate
      @param[in]  x    position in global coordinates
      \return     boundary condition type given by enum in this class
    */
    //    virtual FieldVector<BoundaryConditions::Flags, m> bctype (const FieldVector<DT,n>& x, const Entity& e,
    //            const IntersectionIterator& intersectionIt,
    //            const FieldVector<DT,n>& xi) const = 0;

    virtual FieldVector<BoundaryConditions::Flags, numEq>bctype(
                                                                const FieldVector<Scalar,dim>& globalPos, const Element& element,
                                                                const IntersectionIterator& intersectionIt,
                                                                const FieldVector<Scalar,dim>& localPos) const = 0;

    //! returns index of the primary variable corresponding to the dirichlet boundary condition at the given global coordinate
    /*! returns index of the primary variable corresponding to the dirichlet boundary condition at the given global coordinate
      @param[in]  x    position in global coordinates
      \return     index of the primary variable
    */

    virtual void dirichletIndex(const FieldVector<Scalar,dim>& globalPos, const Element& element,
                                const IntersectionIterator& intersectionIt,
                                const FieldVector<Scalar,dim>& localPos, FieldVector<int,numEq>& dirichletIdx) const
    {
        for (int i = 0; i < numEq; i++)
            dirichletIdx[i]=i;
        return;
    }

    //! evaluate Dirichlet boundary condition at given position
    /*! evaluate Dirichlet boundary condition at given position
      @param[in]  x    position in global coordinates
      \return     boundary condition value
    */
    virtual FieldVector<Scalar,numEq> g(const FieldVector<Scalar,dim>& globalPos, const Element& element,
                                        const IntersectionIterator& intersectionIt,
                                        const FieldVector<Scalar,dim>& localPos) const = 0;

    //! evaluate Neumann boundary condition at given position
    /*! evaluate Neumann boundary condition at given position
      @param[in]  x    position in global coordinates
      \return     boundary condition value
    */
    virtual FieldVector<Scalar,numEq> J(const FieldVector<Scalar,dim>& globalPos, const Element& element,
                                        const IntersectionIterator& intersectionIt,
                                        const FieldVector<Scalar,dim>& localPos) const = 0;

    //! evaluate initial condition at given position
    /*! evaluate initial boundary condition at given position
      @param[in]  x    position in global coordinates
      \return     boundary condition value
    */
    virtual FieldVector<Scalar,numEq> initial(const FieldVector<Scalar,dim>& globalPos,
                                              const Element& element, const FieldVector<Scalar,dim>& localPos) const = 0;

    virtual FieldVector<Scalar,dim> gravity() const = 0;

    //! properties of the wetting (liquid) phase
    /*! properties of the wetting (liquid) phase
      \return    wetting phase
    */
    virtual Fluid& wettingPhase () const
    {
        return wettingPhase_;
    }

    //! properties of the nonwetting (liquid) phase
    /*! properties of the nonwetting (liquid) phase
      \return    nonwetting phase
    */
    virtual Fluid& nonwettingPhase () const
    {
        return nonwettingPhase_;
    }

    //! properties of the soil
    /*! properties of the soil
      \return    soil
    */
    virtual MincLensSoil<Grid, Scalar>& soil () const
    {
        return soil_;
    }

    //! object for definition of material law
    /*! object for definition of material law (e.g. Brooks-Corey, Van Genuchten, ...)
      \return    material law
    */
    virtual TwoPhaseRelations<Grid, Scalar>& materialLaw () const
    {
        return materialLaw_;
    }

    //element-wise return of the values of an Exact solution
    virtual Scalar uExOutVertex(int &ElementIndex, int VariableIndex) const {
        DUNE_THROW(NotImplemented, "Ex(akt) Solution");
        return 0;
    }

    //updates an exact/analytic solution
    virtual void updateExSol(double &dt,
                             BlockVector<FieldVector<Scalar, numEq> > &approxSol) {
        DUNE_THROW(NotImplemented, "Ex(akt) Solution");
        return;
    }

    MincProblem(Fluid& liq1, Fluid& liq2, MincLensSoil<Grid, Scalar>& soil,
                TwoPhaseRelations<Grid,Scalar>& materialLaw = *(new TwoPhaseRelations<Grid,Scalar>),
                const bool exsol = false)
        : exsolution(exsol), wettingPhase_(liq1), nonwettingPhase_(liq2), soil_(soil),
          materialLaw_(materialLaw)
    {     }

    //! always define virtual destructor in abstract base class
    virtual ~MincProblem() {
    }

    const bool exsolution;

protected:
    Fluid& wettingPhase_;
    Fluid& nonwettingPhase_;
    MincLensSoil<Grid, Scalar>& soil_;
    TwoPhaseRelations<Grid, Scalar>& materialLaw_;
};

}
#endif
