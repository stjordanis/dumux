#ifndef DUNE_WATERAIRPROBLEM_HH
#define DUNE_WATERAIRPROBLEM_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dune/grid/io/file/dgfparser/dgfug.hh>

#include <dumux/material/multicomponentrelations.hh>
#include <dumux/material/matrixproperties.hh>
#include <dumux/material/fluids/water_air.hh>

#include <dumux/boxmodels/2p2cni/2p2cniboxmodel.hh>

#define ISOTHERMAL 0

/**
 * @file
 * @brief  Definition of a problem, where air is injected under a low permeable layer
 * @author Bernd Flemisch, Klaus Mosthaf
 */

namespace Dune
{
template <class TypeTag>
class WaterAirProblem;

namespace Properties
{
NEW_TYPE_TAG(WaterAirProblem, INHERITS_FROM(BoxTwoPTwoCNI));

// Set the grid type
SET_TYPE_PROP(WaterAirProblem, Grid, Dune::UGGrid<2>);

// Set the problem property
SET_PROP(WaterAirProblem, Problem)
{
    typedef Dune::WaterAirProblem<TTAG(WaterAirProblem)> type;
};

// Set the wetting phase
SET_TYPE_PROP(WaterAirProblem, WettingPhase, Dune::Liq_WaterAir);

// Set the non-wetting phase
SET_TYPE_PROP(WaterAirProblem, NonwettingPhase, Dune::Gas_WaterAir);

// Set multi-component relations
SET_TYPE_PROP(WaterAirProblem, MultiComp, Dune::CWaterAir)

// Set the soil properties
SET_PROP(WaterAirProblem, Soil)
{
private:
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Grid)) Grid;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
    
public:
    typedef Dune::HomogeneousSoil<Grid, Scalar> type;
};

// Enable gravity
SET_BOOL_PROP(WaterAirProblem, EnableGravity, true);
}


//! class that defines the parameters of an air waterair under a low permeable layer
/*! Problem definition of an air injection under a low permeable layer. Air enters the domain
 * at the right boundary and migrates upwards.
 * Problem was set up using the rect2d.dgf grid.
 *
 *    Template parameters are:
 *
 *    - ScalarT  Floating point type used for scalars
 */
template <class TypeTag = TTAG(WaterAirProblem) >
class WaterAirProblem : public TwoPTwoCNIBoxProblem<TypeTag, WaterAirProblem<TypeTag> >
{
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar))     Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView))   GridView;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Model))      Model;
    typedef typename GridView::Grid                           Grid;

    typedef WaterAirProblem<TypeTag>                ThisType;
    typedef TwoPTwoCNIBoxProblem<TypeTag, ThisType> ParentType;

    // copy some indices for convenience
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(TwoPTwoCIndices)) Indices;
    enum {
        numEq       = GET_PROP_VALUE(TypeTag, PTAG(NumEq)),

        pressureIdx = Indices::pressureIdx,
        switchIdx   = Indices::switchIdx,
#if !ISOTHERMAL
        temperatureIdx = Indices::temperatureIdx,
#endif

        // Phase State
        wPhaseOnly  = Indices::wPhaseOnly,
        nPhaseOnly  = Indices::nPhaseOnly,
        bothPhases  = Indices::bothPhases,

        // Grid and world dimension
        dim         = GridView::dimension,
        dimWorld    = GridView::dimensionworld,
    };

    typedef typename GET_PROP(TypeTag, PTAG(SolutionTypes)) SolutionTypes;
    typedef typename SolutionTypes::PrimaryVarVector        PrimaryVarVector;
    typedef typename SolutionTypes::BoundaryTypeVector      BoundaryTypeVector;

    typedef typename GridView::template Codim<0>::Entity        Element;
    typedef typename GridView::template Codim<dim>::Entity      Vertex;
    typedef typename GridView::IntersectionIterator             IntersectionIterator;
  
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(FVElementGeometry)) FVElementGeometry;

    typedef Dune::FieldVector<Scalar, dim>       LocalPosition;
    typedef Dune::FieldVector<Scalar, dimWorld>  GlobalPosition;
    
public:
    WaterAirProblem(const GridView &gridView)
        : ParentType(gridView)
    {
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
    { return "waterair"; }

#if ISOTHERMAL
    /*!
     * \brief Returns the temperature within the domain.
     *
     * This problem assumes a temperature of 10 degrees Celsius.
     */
    Scalar temperature() const
    {
        return 273.15 + 30; // -> 30°C
    };
#endif

    // \}

    /*!
     * \name Boundary conditions
     */
    // \{

    /*! 
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     */
    void boundaryTypes(BoundaryTypeVector         &values,
                       const Element              &element,
                       const FVElementGeometry    &fvElemGeom,
                       const IntersectionIterator &isIt,
                       int                         scvIdx,
                       int                         boundaryFaceIdx) const
    {
        const GlobalPosition &globalPos
            = element.geometry().corner(scvIdx);
        
        if(globalPos[0] < eps_)
            values = BoundaryConditions::dirichlet;
        else
            values = BoundaryConditions::neumann;
        
#if !ISOTHERMAL
        values[temperatureIdx] = BoundaryConditions::dirichlet;
#endif
    }

    /*! 
     * \brief Evaluate the boundary conditions for a dirichlet
     *        boundary segment.
     *
     * For this method, the \a values parameter stores primary variables.
     */
    void dirichlet(PrimaryVarVector           &values,
                   const Element              &element,
                   const FVElementGeometry    &fvElemGeom,
                   const IntersectionIterator &isIt,
                   int                         scvIdx,
                   int                         boundaryFaceIdx) const
    {
        const GlobalPosition &globalPos
            = element.geometry().corner(scvIdx);
        //                const LocalPosition &localPos
        //                    = DomainTraits::referenceElement(element.geometry().type()).position(dim,scvIdx);

        initial_(values, globalPos);
    }

    /*! 
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each component. Negative values mean
     * influx.
     */
    void neumann(PrimaryVarVector           &values,
                 const Element              &element,
                 const FVElementGeometry    &fvElemGeom,
                 const IntersectionIterator &isIt,
                 int                         scvIdx,
                 int                         boundaryFaceIdx) const
    {
        const GlobalPosition &globalPos
            = element.geometry().corner(scvIdx);

        values = 0;

        // negative values for injection
        Scalar width = this->bboxMax()[0] - this->bboxMin()[0];
        if (globalPos[0] > width - eps_ &&
            globalPos[1] < 5.0 && globalPos[1] < 15.0)
        {
            values[switchIdx] = -1e-3;
        }
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
     * For this method, the \a values parameter stores the rate mass
     * of a component is generated or annihilate per volume
     * unit. Positive values mean that mass is created, negative ones
     * mean that it vanishes.
     */
    void source(PrimaryVarVector        &values,
                const Element           &element,
                const FVElementGeometry &fvElemGeom,
                int                      scvIdx) const
    {
        values = Scalar(0.0);
    }

    /*! 
     * \brief Evaluate the initial value for a control volume.
     *
     * For this method, the \a values parameter stores primary
     * variables.
     */
    void initial(PrimaryVarVector        &values,
                 const Element           &element,
                 const FVElementGeometry &fvElemGeom,
                 int                      scvIdx) const
    {
        const GlobalPosition &globalPos
            = element.geometry().corner(scvIdx);

        initial_(values, globalPos);
    }
    
    /*! 
     * \brief Return the initial phase state inside a control volume.
     */
    int initialPhaseState(const Vertex         &vert,
                          int                  &globalIdx,
                          const GlobalPosition &globalPos) const
    {
        return wPhaseOnly;
    }

    // \}

private:
    // internal method for the initial condition (reused for the
    // dirichlet conditions!)
    void initial_(PrimaryVarVector     &values,
                  const GlobalPosition &globalPos) const
    {
        Scalar densityW = 1000.0;
        values[pressureIdx] = 1e5 + (depthBOR_ - globalPos[1])*densityW*9.81;
        values[switchIdx] = 0.0;
#if !ISOTHERMAL
        values[temperatureIdx] = 283.0 + (depthBOR_ - globalPos[1])*0.03;
#endif
    }

    static const Scalar depthBOR_ = 1000.0; // [m]
    static const Scalar eps_ = 1e-6;
};
} //end namespace

#endif
