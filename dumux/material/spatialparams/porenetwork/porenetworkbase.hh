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
 *
 * \ingroup SpatialParameters
 * \brief The base class for spatial parameters for pore-network models.
 */
#ifndef DUMUX_PNM_SPATIAL_PARAMS_BASE_HH
#define DUMUX_PNM_SPATIAL_PARAMS_BASE_HH

#include <dune/common/fvector.hh>

namespace Dumux::PoreNetwork {

#ifndef DOXYGEN
namespace Detail {
// helper struct detecting if the user-defined spatial params class has a materialLawParamsAtPos function
// for g++ > 5.3, this can be replaced by a lambda
template<class GlobalPosition>
struct hasMaterialLawParamsAtPos
{
    template<class SpatialParams>
    auto operator()(const SpatialParams& a)
    -> decltype(a.materialLawParamsAtPos(std::declval<GlobalPosition>()))
    {}
};

// helper struct detecting if the user-defined spatial params class has a permeabilityAtPos function
// for g++ > 5.3, this can be replaced by a lambda
template<class GlobalPosition>
struct hasPermeabilityAtPos
{
    template<class SpatialParams>
    auto operator()(const SpatialParams& a)
    -> decltype(a.permeabilityAtPos(std::declval<GlobalPosition>()))
    {}
};

template<class GlobalPosition, class SolidSystem>
struct hasInertVolumeFractionAtPos
{
    template<class SpatialParams>
    auto operator()(const SpatialParams& a)
    -> decltype(a.template inertVolumeFractionAtPos<SolidSystem>(std::declval<GlobalPosition>(), 0))
    {}
};

template<class GlobalPosition>
struct hasPorosityAtPos
{
    template<class SpatialParams>
    auto operator()(const SpatialParams& a)
    -> decltype(a.porosityAtPos(std::declval<GlobalPosition>()))
    {}
};

} // end namespace Detail
#endif

/*!
 * \ingroup SpatialParameters
 */

/**
 * \brief The base class for spatial parameters for pore-network models.
 */
template<class GridGeometry, class Scalar, class Implementation>
class BaseSpatialParams
{
    using GridView = typename GridGeometry::GridView;
    using SubControlVolume = typename GridGeometry::SubControlVolume;
    using Element = typename GridView::template Codim<0>::Entity;

    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;
    static constexpr auto dimWorld = GridView::dimensionworld;

public:
    using PermeabilityType = Scalar;

    BaseSpatialParams(std::shared_ptr<const GridGeometry> gridGeometry)
    : gridGeometry_(gridGeometry)
    , gravity_(0.0)
    {
        const bool enableGravity = getParam<bool>("Problem.EnableGravity");
        if (enableGravity)
            gravity_[dimWorld-1]  = -9.81;
    }

    /*!
     *\brief Length of the throat \f$[m]\f$.
     *       Can be solution-dependent.
     *
     *  \param element The finite volume element
     *  \param elemVolVars The element volume variables.
     */
    template<class ElementVolumeVariables>
    Scalar throatLength(const Element& element,
                        const ElementVolumeVariables& elemVolVars) const
    {
        const auto eIdx = gridGeometry().elementMapper().index(element);
        return gridGeometry().throatLength(eIdx);
    }

    /*!
     *\brief Inscribed radius of the throat \f$[m]\f$.
     *       Can be solution-dependent.
     *
     *  \param element The finite volume element
     *  \param elemVolVars The element volume variables.
     */
    template<class ElementVolumeVariables>
    Scalar throatInscribedRadius(const Element& element,
                                 const ElementVolumeVariables& elemVolVars) const
    {
        const auto eIdx = gridGeometry().elementMapper().index(element);
        return gridGeometry().throatInscribedRadius(eIdx);
    }

    /*!
     *\brief Cross-sectional area of the throat \f$[m]\f$.
     *       Can be solution-dependent.
     *
     *  \param element The finite volume element
     *  \param elemVolVars The element volume variables.
     */
    template<class ElementVolumeVariables>
    Scalar throatCrossSectionalArea(const Element& element,
                                   const ElementVolumeVariables& elemVolVars) const
    {
        const auto eIdx = gridGeometry().elementMapper().index(element);
        return gridGeometry().throatCrossSectionalArea(eIdx);
    }

   /*!
    *\brief Inscribed radius of the pore body \f$[m]\f$.
    *       Can be solution-dependent.
    *
    *  \param element The finite volume element
    *  \param scv The sub-control volume
    *  \param elemSol The element solution
    */
    template<class ElementSolutionVector>
    Scalar poreInscribedRadius(const Element& element,
                               const SubControlVolume& scv,
                               const ElementSolutionVector& elemSol) const
    {
        return gridGeometry().poreInscribedRadius(scv.dofIndex());
    }

    /*!
     * \brief Returns a reference to the gridview
     */
    const GridView& gridView() const
    { return gridGeometry().gridView(); }


    /*! Intrinsic permeability tensor K \f$[m^2]\f$ depending
     *  on the position in the domain
     *
     *  \param element The finite volume element
     *  \param scv The sub-control volume
     *
     *  Solution dependent permeability function
     */
    template<class ElementSolutionVector>
    Scalar permeability(const Element& element,
                        const SubControlVolume& scv,
                        const ElementSolutionVector& elemSol) const
    { return 1.0; }

    /*!
     * \brief Returns the acceleration due to gravity \f$\mathrm{[m/s^2]}\f$.
     *
     * The default behaviour is a constant gravity vector;
     * if the <tt>Problem.EnableGravity</tt> parameter is true,
     * \f$\boldsymbol{g} = ( 0,\dots,\ -9.81)^T \f$,
     * else \f$\boldsymbol{g} = ( 0,\dots, 0)^T \f$.
     *
     * \param pos the spatial position at which to evaulate the gravity vector
     */
    const GlobalPosition& gravity(const GlobalPosition& pos) const
    { return gravity_; }

    //! The finite volume grid geometry
    const GridGeometry& gridGeometry() const
    { return *gridGeometry_; }

    /*!
     * \brief Function for defining the porosity.
     *        That is possibly solution dependent.
     * \note this can only be used for solids with one inert component
     *       (see inertVolumeFraction for the more general interface)
     * \param element The current element
     * \param scv The sub-control volume inside the element.
     * \param elemSol The solution at the dofs connected to the element.
     * \return the porosity
     */
    template<class ElementSolution>
    Scalar porosity(const Element& element,
                    const SubControlVolume& scv,
                    const ElementSolution& elemSol) const
    {
        static_assert(decltype(isValid(Detail::hasPorosityAtPos<GlobalPosition>())(this->asImp_()))::value," \n\n"
        "   Your spatial params class has to either implement\n\n"
        "         Scalar porosityAtPos(const GlobalPosition& globalPos) const\n\n"
        "   or overload this function\n\n"
        "         template<class ElementSolution>\n"
        "         Scalar porosity(const Element& element,\n"
        "                         const SubControlVolume& scv,\n"
        "                         const ElementSolution& elemSol) const\n\n");

        return asImp_().porosityAtPos(scv.center());
    }

    Scalar porosityAtPos(const GlobalPosition& globalPos) const
    { return 1.0; }

    /*!
     * \brief Function for defining the solid volume fraction.
     *        That is possibly solution dependent.
     *
     * \param element The current element
     * \param scv The sub-control volume inside the element.
     * \param elemSol The solution at the dofs connected to the element.
     * \param compIdx The solid component index
     * \return the volume fraction of the inert solid component with index compIdx
     *
     * \note this overload is enable if there is only one inert solid component and the
     *       user didn't choose to implement a inertVolumeFractionAtPos overload.
     *       It then forwards to the simpler porosity interface.
     *       With more than one solid components or active solid components (i.e. dissolution)
     *       please overload the more general inertVolumeFraction/inertVolumeFractionAtPos interface.
     */
    template<class SolidSystem, class ElementSolution,
             typename std::enable_if_t<SolidSystem::isInert()
                                       && SolidSystem::numInertComponents == 1
                                       && !decltype(isValid(Detail::hasInertVolumeFractionAtPos<GlobalPosition, SolidSystem>())(std::declval<Implementation>()))::value,
                                       int> = 0>
    Scalar inertVolumeFraction(const Element& element,
                               const SubControlVolume& scv,
                               const ElementSolution& elemSol,
                               int compIdx) const
    {
        return 1.0 - asImp_().porosity(element, scv, elemSol);
    }

    // specialization if there are no inert components at all
    template<class SolidSystem, class ElementSolution,
             typename std::enable_if_t<SolidSystem::numInertComponents == 0, int> = 0>
    Scalar inertVolumeFraction(const Element& element,
                               const SubControlVolume& scv,
                               const ElementSolution& elemSol,
                               int compIdx) const
    {
        return 0.0;
    }

    // the more general interface forwarding to inertVolumeFractionAtPos
    template<class SolidSystem, class ElementSolution,
             typename std::enable_if_t<(SolidSystem::numInertComponents > 1) ||
                                       (
                                            (SolidSystem::numInertComponents > 0) &&
                                            (
                                                !SolidSystem::isInert()
                                                || decltype(isValid(Detail::hasInertVolumeFractionAtPos<GlobalPosition, SolidSystem>())
                                                        (std::declval<Implementation>()))::value
                                            )
                                        ),
                                        int> = 0>
    Scalar inertVolumeFraction(const Element& element,
                               const SubControlVolume& scv,
                               const ElementSolution& elemSol,
                               int compIdx) const
    {
        static_assert(decltype(isValid(Detail::hasInertVolumeFractionAtPos<GlobalPosition, SolidSystem>())(this->asImp_()))::value," \n\n"
        "   Your spatial params class has to either implement\n\n"
        "         template<class SolidSystem>\n"
        "         Scalar inertVolumeFractionAtPos(const GlobalPosition& globalPos, int compIdx) const\n\n"
        "   or overload this function\n\n"
        "         template<class SolidSystem, class ElementSolution>\n"
        "         Scalar inertVolumeFraction(const Element& element,\n"
        "                                    const SubControlVolume& scv,\n"
        "                                    const ElementSolution& elemSol,\n"
        "                                    int compIdx) const\n\n");

        return asImp_().template inertVolumeFractionAtPos<SolidSystem>(scv.center(), compIdx);
    }

protected:
    Implementation &asImp_()
    { return *static_cast<Implementation*>(this); }

    const Implementation &asImp_() const
    { return *static_cast<const Implementation*>(this); }

private:
    std::shared_ptr<const GridGeometry> gridGeometry_;
    GlobalPosition gravity_; //!< The gravity vector
};

} // namespace Dumux::PoreNetwork

#endif
