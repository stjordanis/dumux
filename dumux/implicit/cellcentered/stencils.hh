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
/*!
 * \file
 * \brief Implements the notion of stencils for cell-centered models
 */
#ifndef DUMUX_CC_STENCILS_HH
#define DUMUX_CC_STENCILS_HH

#include <set>
#include <dumux/implicit/cellcentered/properties.hh>

namespace Dumux
{

/*!
 * \brief Element-related stencils
 */
template<class TypeTag>
class CCElementStencils
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using IndexType = typename GridView::IndexSet::IndexType;
    using Element = typename GridView::template Codim<0>::Entity;
    // TODO a separate stencil class all stencils can derive from?
    using Stencil = std::vector<IndexType>;
public:
    void update(const Problem& problem, const Element& element)
    {
        elementStencil_.clear();
        for (auto&& scvf : problem.model().fvGeometries(element).scvfs())
        {
            auto&& fluxStencil = problem.model().fluxVars(scvf).stencil();
            elementStencil_.insert(elementStencil_.end(), fluxStencil.begin(), fluxStencil.end());
        }
        // make values in elementstencil unique
        std::sort(elementStencil_.begin(), elementStencil_.end());
        elementStencil_.erase(std::unique(elementStencil_.begin(), elementStencil_.end()), elementStencil_.end());

        neighborStencil_ = elementStencil_;
        for (auto it = neighborStencil_.begin(); it != neighborStencil_.end(); ++it)
        {
            if (*it == problem.elementMapper().index(element))
            {
                neighborStencil_.erase(it);
                break;
            }
        }
    }

    //! The full element stencil (all element this element is interacting with)
    const Stencil& elementStencil() const
    {
        return elementStencil_;
    }

    //! //! The full element stencil without this element
    const Stencil& neighborStencil() const
    {
        return neighborStencil_;
    }

private:
    Stencil elementStencil_;
    Stencil neighborStencil_;
};

/*!
 * \ingroup CCModel
 * \brief The global stencil container class
 */
template<class TypeTag>
class CCStencilsVector
{
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);

public:
    void update(const Problem& problem)
    {
        problemPtr_ = &problem;
        elementStencils_.resize(problem.gridView().size(0));
        for (const auto& element : elements(problem.gridView()))
        {
            auto eIdx = problem.elementMapper().index(element);
            elementStencils_[eIdx].update(problem, element);
        }
    }

    //! overload for elements
    template <class Entity>
    typename std::enable_if<Entity::codimension == 0, const CCElementStencils<TypeTag>&>::type
    get(const Entity& entity) const
    {
        return elementStencils_[problemPtr_->elementMapper().index(entity)];
    }

private:
    std::vector<CCElementStencils<TypeTag>> elementStencils_;
    const Problem* problemPtr_;
};

} // end namespace

#endif
