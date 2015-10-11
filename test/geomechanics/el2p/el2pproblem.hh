// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
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
/**
 * \file
 * \brief Definition of a problem, for the two-phase flow linear elasticity problem:
 * Problem definition for the deformation of an elastic solid.
 */
#ifndef DUMUX_EL2P_TESTPROBLEM_HH
#define DUMUX_EL2P_TESTPROBLEM_HH

#include <dune/common/version.hh>
#include <dune/grid/io/file/dgfparser/dgfyasp.hh>
#include <dune/pdelab/finiteelementmap/qkfem.hh>

#include <dumux/material/fluidsystems/brineco2fluidsystem.hh>
#include <dumux/implicit/common/implicitporousmediaproblem.hh>
#include <dumux/geomechanics/el2p/el2pmodel.hh>
#include <dumux/geomechanics/el2p/el2pamgbackend.hh>

#include "el2pco2tables.hh"
#include "el2pspatialparams.hh"

namespace Dumux
{
template<class TypeTag>
class El2P_TestProblem;


// initial conditions for momentum balance equation
template<class GridView, class Scalar, int dim>
class InitialDisplacement;

// initial conditions for mass balance equations
template<class GridView, class Scalar>
class InitialPressSat;

namespace Properties {
NEW_TYPE_TAG(El2P_TestProblem, INHERITS_FROM(BoxModel, BoxElasticTwoP, El2PSpatialParams));
NEW_PROP_TAG(InitialDisplacement); //!< The initial displacement function
NEW_PROP_TAG(InitialPressSat); //!< The initial pressure and saturation function

// Set the grid type
SET_TYPE_PROP(El2P_TestProblem, Grid, Dune::YaspGrid<3>);

SET_PROP(El2P_TestProblem, PressureFEM)
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

public:
    typedef Dune::PDELab::QkLocalFiniteElementMap<GridView,Scalar,Scalar,1>  type;
};

SET_PROP(El2P_TestProblem, DisplacementFEM)
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;

public:
    typedef Dune::PDELab::QkLocalFiniteElementMap<GridView,Scalar,Scalar,1>  type;
};

// Set the problem property
SET_TYPE_PROP(El2P_TestProblem, Problem, El2P_TestProblem<TypeTag>);

// Set fluid configuration
SET_PROP(El2P_TestProblem, FluidSystem)
{
    typedef Dumux::BrineCO2FluidSystem<TypeTag> type;
};

// Set the CO2 table to be used; in this case not the the default table
SET_TYPE_PROP(El2P_TestProblem, CO2Table, Dumux::El2P::CO2Tables);
// Set the salinity mass fraction of the brine in the reservoir
SET_SCALAR_PROP(El2P_TestProblem, ProblemSalinity, 1e-1);

// Set the soil properties
SET_TYPE_PROP(El2P_TestProblem, SpatialParams, El2PSpatialParams<TypeTag>);

// Set the initial displacement function
SET_PROP(El2P_TestProblem, InitialDisplacement)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    enum{dim = GridView::dimension};
public:
    typedef Dumux::InitialDisplacement<GridView, Scalar, dim> type;
};

// Set the initial pressure and saturation function
SET_PROP(El2P_TestProblem, InitialPressSat)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
public:
    typedef Dumux::InitialPressSat<GridView, Scalar> type;
};

SET_SCALAR_PROP(El2P_TestProblem, NewtonMaxRelativeShift, 1e-5);

// use the algebraic multigrid
SET_TYPE_PROP(El2P_TestProblem, LinearSolver, Dumux::El2PAMGBackend<TypeTag>);

// central differences to calculate the jacobian by default
SET_INT_PROP(El2P_TestProblem, ImplicitNumericDifferenceMethod, 0);

// write the stress and displacement output according to rock mechanics
// sign convention (compressive stresses > 0)
SET_BOOL_PROP(El2P_TestProblem, VtkRockMechanicsSignConvention, true);
}

/*!
 * \ingroup ElTwoPBoxProblems
 *
 * \brief Problem definition for a two-phase flow process
 * in an elastic deformable matrix.
 *
 * This problem simulates an injection of CO2 into the center of a cube with 1000 m x 1000 m x 1000 m
 * dimension. The bottom boundary of this cube is in 2000 m depth. The initialization period is 1e6 s,
 * the real injection period is 1e6 s, the initial timestep is 10 s.
 * Apart from the pressure and the saturation distribution this problems solves for the changes in
 * solid displacement (ux, uy, uz [m]) due to injection. Based on the solid displacement vector
 * the injection-induced changes in the strain and stress tensors are evaluated.
 * Further the porosity and permeability are functions of the solid displacement.
 *
 * During an initialization period of length tInit [s] the pressure field is initialized.
 *
 * After the initialization the real simulation starts and the pressure field from the initialization
 * period is applied as initial condition and for the definition of the lateral Dirichlet
 * boundary conditions. The solid  displacement field is set to zero and the CO2 injection is started.
 */
template<class TypeTag = TTAG(El2P_TestProblem)>
class El2P_TestProblem : public ImplicitPorousMediaProblem<TypeTag>
{
    typedef ImplicitPorousMediaProblem<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Grid) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;

    enum {
        // Grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld,
    };

    enum {
        // indices of the primary variables
            pressureIdx = Indices::pwIdx,
            saturationIdx = Indices::snIdx,
            uxIdx = Indices::uxIdx,
            uyIdx = Indices::uyIdx,
            uzIdx = Indices::uzIdx

    };
    enum {
        // indices of the equations+
            contiWEqIdx = Indices::contiWEqIdx,
            contiNEqIdx = Indices::contiNEqIdx
    };


    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::Intersection Intersection;
    typedef typename GridView::template Codim<dim>::Iterator VertexIterator;
    typedef typename GET_PROP_TYPE(TypeTag, VertexMapper) VertexMapper;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;

    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename Grid::ctype CoordScalar;
    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;
    typedef Dune::BlockVector<GlobalPosition> InitialStressField;

    typedef typename GET_PROP_TYPE(TypeTag, LocalFEMSpace) LocalFEMSpace;

    typedef typename GET_PROP_TYPE(TypeTag, CO2Table) CO2Table;
    typedef Dumux::CO2<Scalar, CO2Table> CO2;

public:
    /*!
     * \brief The constructor
     *
     * \param timeManager The time manager
     * \param gridView The grid view
     * \param tInitEnd End of initialization period
     */
    El2P_TestProblem(TimeManager &timeManager,
                    const GridView &gridView,
                    const Scalar tInitEnd)
        : ParentType(timeManager, gridView),
        gridView_(gridView),
        vertexMapper_(gridView)
    {
        std::cout << "El2P_TestProblem: Initializing the fluid system for the el2p model\n";

        // initialize the tables of the fluid system
        FluidSystem::init(/*Tmin=*/273,
                          /*Tmax=*/400,
                          /*nT=*/120,
                          /*pmin=*/1e5,
                          /*pmax=*/1e8,
                          /*np=*/200);

        // resize the pressure field vector with the number of vertices
        pInit_.resize(gridView.size(dim));
        // fill the pressure field vector with zeros
        std::fill( pInit_.begin(), pInit_.end(), 0.0 );

        // variable which determines if output should be written (initially set to false)
        output_ = false;
        // define if current run is initialization run
        // (initially set to true, will be set to false if initialization is over)
        initializationRun_ = true;
        // defines if feedback from geomechanics on flow is taken into account or not
        // (usually the coupling is switched off for the initialization run)
        coupled_ = false;
        // set initial episode length equal to length of initialization period
        this->timeManager().startNextEpisode(tInitEnd);
        // transfer the episode index to spatial parameters
        // (during intialization episode hydraulic different parameters might be applied)
        this->spatialParams().setEpisode(this->timeManager().episodeIndex());
    }

    // note: pInit is < 0 (just due to geomechanics sign convention applied here)
    // initialize the pressure field for initialization run
    // first an approximate hydrostatic pressure field is calculated based on an
    // averaged density. Then the model runs for the initialization period and
    // calculates the real hydrostatic pressure distribution based on the real
    // density distribution. The calculated pressure field is than applied for
    // initialization of the actual model run and for the pressure Dirichlet boundary values.
    void initializePressure()
    {
        VertexIterator vIt = gridView_.template begin<dim>();
        VertexIterator vEndIt = gridView_.template end<dim>();
        for(; vIt != vEndIt; ++vIt)
        {
            int vIdxGlobal = vertexMapper_.index(*vIt);
            GlobalPosition globalPos = (*vIt).geometry().corner(0);

            // initial approximate pressure distribution at start of initialization run
            pInit_[vIdxGlobal] = -(1.013e5 + (depthBOR_ - globalPos[2]) * brineDensity_ * 9.81);
        }
    }

    // allows to change the coupled_ variable which defines if geomechanical feedback on flow is taken
    // into account
    void setCoupled(bool coupled)
    {
        coupled_ = coupled;
    }

    // returns the coupled_ variable which defines if geomechanical feedback on flow is taken
    // into account
    const bool coupled() const
    {
        return coupled_;
    }

    // allows to change the output_ variable which defines if output is written
    void setOutput(bool output)
    {
        output_ = output;
    }

    // note: pInit is < 0 (just due to geomechanics sign convention applied here)
    // function which is called after the initialization run in
    // order to fill the pressure field vector pInit_ with the
    // pressure result of the initialization
    void setPressure()
    {
        initializationRun_ = false; // initialization run is now finished

        this->setInitializationRun(initializationRun_);
        std::cout<<"El2P_TestProblem: initialized pressure field copied to pInit_"<<std::endl;
        VertexIterator vIt = gridView_.template begin<dim>();
        VertexIterator vEndIt = gridView_.template end<dim>();
        for(; vIt != vEndIt; ++vIt)
        {
            int vIdxGlobal = vertexMapper_.index(*vIt);
            //
            pInit_[vIdxGlobal] = -this->model().curSol().base()[vIdxGlobal*2][0];
        }
    }

    // returns the initializationRun_ variable which defines if this is an initialization run
    bool initializationRun()
    {
        return initializationRun_;
    }

    // allows to set the initializationRun_ variable which defines if this is an initialization run
    void setInitializationRun(bool initializationRun)
    {
        initializationRun_ = initializationRun;
    }

    // function which returns an in-situ stress field that needs to be provided
    // for the principal stress calculation
    GlobalPosition initialStress(const GlobalPosition globalPos, const int dofIdxGlobal) const
    {
      GlobalPosition stress;
      Scalar porosity, rockDensity, gravity;
      gravity = -this->gravity()[2];
      porosity = this->spatialParams().porosity(globalPos);
      rockDensity = this->spatialParams().rockDensity(globalPos);

      // initial total stress field here assumed to be isotropic, lithostatic
      stress[0] = brineDensity_ * porosity * gravity * (depthBOR_ - globalPos[dim-1])
                  + (1 - porosity) * rockDensity * gravity * (depthBOR_ - globalPos[dim-1]);
      if(dim >=2)
      stress[1] = brineDensity_ * porosity * gravity * (depthBOR_ - globalPos[dim-1])
                  + (1 - porosity) * rockDensity * gravity * (depthBOR_ - globalPos[dim-1]);
      if(dim == 3)
      stress[2] = brineDensity_ * porosity * gravity * (depthBOR_ - globalPos[dim-1])
                  + (1 - porosity) * rockDensity * gravity * (depthBOR_ - globalPos[dim-1]);

      return stress;
    }
    /*!
     * \name Problem parameters
     */
    // \{

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     */
    const char *name() const
    {
        return "el2p";
    }

    /*!
     * \brief Returns the temperature within the domain.
     *
     * This problem assumes a temperature of 10 degrees Celsius at the ground surface
     * and a geothermal gradient of 0.03 K/m.
     */
    Scalar temperatureAtPos(const GlobalPosition &globalPos) const
    {
        Scalar T;
        T = 283.15 + (depthBOR_ - globalPos[2]) * 0.03;

        return T;
    };


    // returns the bottom of reservoir value (depth in m)
    const Scalar depthBOR() const
    {
        return depthBOR_;
    }

    // note: pInit is < 0 (just due to geomechanics sign convention applied here)
    // function which returns the initialized pressure at an arbitrary location within the element
    // called from finite element method (el2plocaloperator.hh) and evaluated at Gauss points
    Scalar pInit(const GlobalPosition& globalPos, const GlobalPosition& localPos, const Element& element) const
    {
        Scalar pValue = 0.0;

        typename El2P_TestProblem<TypeTag>::LocalFEMSpace feMap(this->gridView());
        const typename LocalFEMSpace::Traits::FiniteElementType
        &localFiniteElement = feMap.find(element.geometry().type());
        typedef Dune::FieldVector<CoordScalar, 1> ShapeValue;
        std::vector<ShapeValue> shapeVal;
        localFiniteElement.localBasis().evaluateFunction(localPos, shapeVal);

#if DUNE_VERSION_NEWER(DUNE_COMMON, 2, 4)
        for (int i = 0; i < element.subEntities(dim); i++)
#else
        for (int i = 0; i < element.template count<dim>(); i++)
#endif
        {
            int vIdxGlobal = this->vertexMapper().subIndex(element, i, dim);
            pValue += pInit_[vIdxGlobal] * shapeVal[i];
        }

        return pValue;
    }

    // note: pInit is < 0
    // function which returns initial pressure distribution
    std::vector<Scalar> pInit()
    {
        return pInit_;
    }

    // returns true if the current solution should be written to
    // disk (i.e. as a VTK file)
    // during initialization no output is written
    // during actual simulation output is written initially and
    // at episode/simulation end
    bool shouldWriteOutput()
    {
        if(output_ == false)
            return false;
        else
            return true;
    }

    // returns true if the current solution should be written to
    // disk (i.e. as a drs file)
    bool shouldWriteRestartFile() const
    {
        if(output_ == false)
            return false;
        else
            return true;
    }

    // \}

    /*!
     * \name Boundary conditions
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary control volume.
     *
     * \param values The boundary types for the conservation equations
     * \param vertex The vertex on the boundary for which the
     *               conditions needs to be specified
     *
     * This function calls boundaryTypes with the global position as argument
     */
    void boundaryTypes(BoundaryTypes &values, const Vertex &vertex) const
    {
        const GlobalPosition globalPos = vertex.geometry().center();

        boundaryTypes(values, globalPos);
    }
    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary control volume.
     *
     * \param values The boundary types for the conservation equations
     * \param globalPos The center of the finite volume which ought to be set.
     *
     *               This function is called directly from dumux/geomechanics/el2p/el2plocaloperator.hh
     *               If it is renamed to boundaryTypesAtPos it should be adjusted there as well.
     */
    void boundaryTypes(BoundaryTypes &values, const GlobalPosition& globalPos) const
    {
        values.setAllNeumann();

        // The solid displacement normal to the lateral boundaries is fixed.
        if(globalPos[0] < eps_ || globalPos[0] > this->bBoxMax()[0]-eps_)
        {
            values.setDirichlet(uxIdx);
            if(initializationRun_ == false)
            {
                values.setDirichlet(pressureIdx);
                values.setDirichlet(saturationIdx);
            }
        }
        // The solid displacement normal to the lateral boundaries is fixed.
        if(globalPos[1] < eps_ || globalPos[1] > this->bBoxMax()[1]-eps_)
        {
            values.setDirichlet(uyIdx);
            if(initializationRun_ == false)
            {
                values.setDirichlet(pressureIdx);
                values.setDirichlet(saturationIdx);
            }
        }

        // Lower boundary closed for brine and CO2 flux, uz is fixed.
        if(globalPos[2] < eps_)
        {
            values.setDirichlet(uzIdx);
        }

        // for the initialization run the pressure and saturation
        // values are only given at the top boundary.
        if(globalPos[2] > this->bBoxMax()[2]-eps_)
        {
            values.setDirichlet(pressureIdx);
            values.setDirichlet(saturationIdx);
        }
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param values The dirichlet values for the primary variables
     * \param vertex The vertex representing the "half volume on the boundary"
     *
     * For this method, the \a values parameter stores primary variables.
     */
    void dirichlet(PrimaryVariables &values, const Vertex &vertex) const
    {
        const GlobalPosition globalPos = vertex.geometry().center();

        dirichlet(values, globalPos);
        values[0] = -pInit_[this->vertexMapper().index(vertex)];
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        control volume.
     *
     * \param values The dirichlet values for the primary variables
     * \param globalPos The center of the finite volume which ought to be set.
     *
     * This function is called directly from dumux/geomechanics/el2p/el2plocaloperator.hh
     * If it is renamed to dirichletAtPos it should be adjusted there as well.
     */
    void dirichlet(PrimaryVariables &values, const GlobalPosition& globalPos) const
    {
        values = 0.0;
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * This is the method for the case where the Neumann condition is
     * potentially solution dependent and requires some box method
     * specific things.
     *
     * \param values The neumann values for the conservation equations in units of
     *                 \f$ [ \textnormal{unit of conserved quantity} / (m^2 \cdot s )] \f$
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry in the box scheme
     * \param intersection The intersection between element and boundary
     * \param scvIdx The local vertex index
     * \param boundaryFaceIdx The index of the boundary face
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each phase. Negative values mean influx.
     */
    void neumann(PrimaryVariables &values,
            const Element &element,
            const FVElementGeometry &fvGeometry,
            const Intersection &intersection,
            int scvIdx,
            int boundaryFaceIdx) const
    {
        const GlobalPosition globalPos = fvGeometry.boundaryFace[boundaryFaceIdx].ipGlobal;

        neumann(values, globalPos);
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * \param values The neumann values for the conservation equations [kg / (m^2 *s )]
     * \param globalPos The position of the integration point of the boundary segment.
     *
     * This function is called directly from dumux/geomechanics/el2p/el2plocaloperator.hh
     * If it is renamed to neumannAtPos it should be adjusted there as well.
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each phase. Negative values mean influx.
     */
    void neumann(PrimaryVariables &values, const GlobalPosition& globalPos) const
    {
        values = 0;
    }
    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluate the source term for all phases within a given
     *        sub-control-volume.
     *
     * \param values The source values for the conservation equations in units of
     *                 \f$ [ \textnormal{unit of conserved quantity} / (m^3 \cdot s )] \f$
     * \param element The finite element
     * \param fvGeometry The finite-volume geometry in the box scheme
     * \param scvIdx The local vertex index
     *
     * For this method, the \a values parameter stores the rate mass
     * generated or annihilate per volume unit. Positive values mean
     * that mass is created, negative ones mean that it vanishes.
     */
    void source(PrimaryVariables &values,
            const Element &element,
            const FVElementGeometry &fvGeometry,
            int scvIdx) const
    {
        const GlobalPosition globalPos = element.geometry().corner(scvIdx);

        source(values, globalPos);
    }

    /*!
     * \brief Evaluate the source term for all phases within a given
     *        sub-control-volume.
     *
     * \param values The source values for the conservation equations in units of
     *                 \f$ [ \textnormal{unit of conserved quantity} / (m^3 \cdot s )] \f$
     * \param globalPos The position of the integration point of the boundary segment.
     *
     * For this method, the \a values parameter stores the rate mass
     * generated or annihilate per volume unit. Positive values mean
     * that mass is created, negative ones mean that it vanishes.
     */
    void source(PrimaryVariables &values, const GlobalPosition& globalPos) const
    {
        values = 0.0;
        if(initializationRun_ == false){
        if(globalPos[0] > 490 && globalPos[0] < 510
                        && globalPos[1] > 490 && globalPos[1] < 510
                        && globalPos[2] > 490 && globalPos[2] < 510)
        values[saturationIdx] = 1.e-5; // injection
        }
    }

    // \}

    /*!
     * \brief Transfer episode index to spatial parameters in order to apply different
     * hydraulic parameters during pressure initialization
     */
    void preTimeStep()
    {
        this->spatialParams().setEpisode(this->timeManager().episodeIndex());
    }

    /*!
     * \brief Write mass balance information for both fluid phases
     */
    void postTimeStep()
    {
        PrimaryVariables mass;
        this->model().globalStorage(mass);
        double time = this->timeManager().time()+this->timeManager().timeStepSize();

        // Write mass balance information for rank 0
        if (this->gridView().comm().rank() == 0) {
            std::cout << "TIME, MASS NPhase (kg), MASS WPhase (kg): \n"
            <<"mass: "
            <<time<< " , "
            <<mass[1] << " , "
            <<mass[0]
            <<"\n"
            <<"***************************************" <<std::endl;
        }


    }

    /*!
     * \brief Define length of next episode
     */
    void episodeEnd()
    {
        this->timeManager().startNextEpisode(episodeLength_);
    }

private:
    static constexpr Scalar eps_ = 3e-6;
    static constexpr Scalar depthBOR_ = 2000;
    static constexpr Scalar brineDensity_ = 1059;
    static constexpr Scalar episodeLength_ = 1e5;

    std::vector<Scalar> pInit_;
    GridView gridView_;
    VertexMapper vertexMapper_;
    Scalar dt_;
public:
    bool initializationRun_, coupled_, output_;
    InitialStressField initialStressField_;
};


/*!
 * \ingroup ElTwoPBoxProblems
 *
 * \brief Initial conditions for momentum balance equation.
 *
 * Set initial conditions for solution of momentum balance equation
 * i.e. initialize solid displacement
 * This function is called from dumux/geomechanics/el2p/el2pmodel.hh
 *  * This function is called from dumux/geomechanics/el2p/el2pmodel.hh.
 *
 * The primary variables are initialized two times:
 * 1. before the initialization run.
 * 2. at the start of the actual simulation the solid displacement values which have
 *    changed during initialization of the pressure field are set to zero again.
 *
 */
template<typename GridView, typename Scalar, int dim>
class InitialDisplacement :
public Dune::PDELab::AnalyticGridFunctionBase<
    Dune::PDELab::AnalyticGridFunctionTraits<GridView,Scalar,dim>,
    InitialDisplacement<GridView,Scalar,dim> >
{
public:
    typedef Dune::PDELab::AnalyticGridFunctionTraits<GridView,Scalar,dim> Traits;
    typedef Dune::PDELab::AnalyticGridFunctionBase<Traits, InitialDisplacement<GridView,Scalar,dim> > BaseT;

    typedef typename Traits::DomainType DomainType;
    typedef typename Traits::RangeType RangeType;

    /*!
     * \brief The constructor
     *
     * \param gridView The grid view
     */
    InitialDisplacement(const GridView & gridView) : BaseT(gridView) {}

    /*!
     * \brief Evaluate initial conditions for the momentum balance equation.
     *
     * \param position The position of the vertex
     * \param values The initial solid displacement vector at the vertex
     */
    inline void evaluateGlobal(const DomainType & position, RangeType & values) const
    {
        values = 0;
    }
};

/*!
 * \ingroup ElTwoPBoxProblems
 *
 * \brief Initial conditions for mass balance equations.
 *
 * Set initial conditions for solution of the mass balance equations
 * i.e. initialize wetting phase pressure and nonwetting phase saturation
 *
 * This function is called from dumux/geomechanics/el2p/el2pmodel.hh.
 * The primary variables are initialized two times:
 * 1. before the initialization run.
 * 2. at the start of the actual simulation applying pressure field
 *    calculated during initialization
 *
 */
template<typename GridView, typename Scalar>
class InitialPressSat :
public Dune::PDELab::AnalyticGridFunctionBase<
    Dune::PDELab::AnalyticGridFunctionTraits<GridView,Scalar,2>,
    InitialPressSat<GridView,Scalar> >
{
public:
    typedef Dune::PDELab::AnalyticGridFunctionTraits<GridView,Scalar,2> Traits;
    typedef Dune::PDELab::AnalyticGridFunctionBase<Traits, InitialPressSat<GridView,Scalar> > BaseT;

    typedef typename Traits::DomainType DomainType;
    typedef typename Traits::RangeType RangeType;

    enum {
        // indices of the primary variables
            pressureIdx = 0,
            saturationIdx = 1
    };

    typedef typename Dune::MultipleCodimMultipleGeomTypeMapper<GridView,
                        Dune::MCMGVertexLayout> VertexMapper;

    typedef typename GridView::template Codim<GridView::dimension>::Iterator
                        VertexIterator;

    /*!
     * \brief The constructor
     *
     * \param gridView The grid view
     */
    InitialPressSat(const GridView & gridView) : BaseT(gridView) , gridView_(gridView), vertexMapper_(gridView)
    {
        // resize the pressure field vector with the number of vertices
        pInit_.resize(gridView.size(GridView::dimension));
        // fill the pressure field vector with zeros
        std::fill(pInit_.begin(), pInit_.end(), 0.0);
    }

    /*!
     * \brief Evaluate initial conditions for the mass balance equations.
     *
     * \param position The position of the vertex
     * \param values The initial pressure and saturation values at the vertex
     *
     * This function applies the pressure field pInit_ which is defined
     * in the problem.
     */
    inline void evaluateGlobal(const DomainType & position, RangeType & values) const
    {
            bool valueSet;
            VertexIterator vIt =
                            gridView_.template begin<GridView::dimension> ();
            VertexIterator vEndIt =
                            gridView_.template end<GridView::dimension> ();
            valueSet = false;

            // loop over all vertices
            for (; vIt != vEndIt; ++vIt)
            {
#if DUNE_VERSION_NEWER(DUNE_COMMON, 2, 4)
                // get global index of current vertex
                int vIdxGlobal = vertexMapper_.index(*vIt);
#else
                int vIdxGlobal = vertexMapper_.map(*vIt);
#endif
                Dune::FieldVector<double, 3> globalPos =
                                (*vIt).geometry().corner(0);

                // compare coordinates of current vertex with position coordinates
                if (globalPos[0] >= position[0] - eps_ && globalPos[0] <= position[0] + eps_
                                && globalPos[1] >= position[1] - eps_ && globalPos[1]
                                <= position[1] + eps_ && globalPos[2] >= position[2] - eps_
                                && globalPos[2] <= position[2] + eps_)
                {
                    // if coordinates are identical write the pressure value for this
                    // vertex (with index vIdxGlobal) into the values vector
                    values[pressureIdx] = pInit_[vIdxGlobal];
                    // the value of this vertex is set
                    valueSet = true;
                }
            }

            // check if the pressure value for this vertex has been initialized
            if (valueSet == false)
            {
                std::cout << " pressure value not initialized correctly "
                                << std::endl;
            }

            // initialize saturation values
            values[saturationIdx] = 0;
        }

    /*!
     * \brief Fill the vector pInit_ for initialization
     *
     * \param pInit The pressure field vector defined in the problem class
     *
     * This function is called from dumux/geomechanics/el2p/el2pmodel.hh.
     */
        void setPressure(std::vector<Scalar> pInit)
        {
            std::cout << "InitialPressSat: setPressure function called" << std::endl;
            VertexIterator vIt =
                            gridView_.template begin<GridView::dimension> ();
            VertexIterator vEndIt =
                            gridView_.template end<GridView::dimension> ();
            for (; vIt != vEndIt; ++vIt)
            {
                int vIdxGlobal = vertexMapper_.index(*vIt);
                pInit_[vIdxGlobal] = -pInit[vIdxGlobal];
            }
        }

private:
    static constexpr Scalar eps_ = 3e-6;
    static constexpr Scalar depthBOR_ = 2000;
    std::vector<Scalar> pInit_;
    GridView gridView_;
    VertexMapper vertexMapper_;
};

} //end namespace

#endif
