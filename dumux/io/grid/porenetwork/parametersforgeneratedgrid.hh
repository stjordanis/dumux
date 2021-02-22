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
 * \ingroup InputOutput
 * \brief Helper class to assign parameters to a generated grid
 */
#ifndef DUMUX_IO_PARAMETERS_FOR_GENERATED_GRID
#define DUMUX_IO_PARAMETERS_FOR_GENERATED_GRID

#include <algorithm>
#include <array>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include <dune/common/exceptions.hh>
#include <dumux/common/parameters.hh>
#include <dumux/geometry/intersectspointgeometry.hh>
#include <dumux/porenetworkflow/common/throatproperties.hh>
#include <dumux/porenetworkflow/common/poreproperties.hh>

namespace Dumux::PoreNetwork {

/*!
 * \ingroup InputOutput
 * \brief Helper class to assign parameters to a generated grid
 */
template <class Grid, class Scalar>
class ParametersForGeneratedGrid
{
    using GridView = typename Grid::LeafGridView;
    using Element = typename Grid::template Codim<0>::Entity;
    using GlobalPosition = typename Element::Geometry::GlobalCoordinate;

    static constexpr auto dim = Grid::dimension;
    static constexpr auto dimWorld = Grid::dimensionworld;
    using BoundaryList = std::array<int, 2*dimWorld>;

public:

    ParametersForGeneratedGrid(const GridView& gridView, const std::string& paramGroup)
    : gridView_(gridView)
    , paramGroup_(paramGroup)
    , priorityList_(getPriorityList_())
    {
        computeBoundingBox_();
        boundaryFaceIndex_ = getBoundaryFacemarkerInput_();
    }

    /*!
     * \brief Returns the boundary face marker index at given position
     *
     * \param pos The current position
     */
    int boundaryFaceMarkerAtPos(const GlobalPosition& pos) const
    {
        // set the priority which decides the order the vertices on the boundary are indexed
        // by default, vertices on min/max faces in x direcetion have the highest priority, followed by y and z
        for (auto boundaryIdx : priorityList_)
        {
            if (onBoundary_(pos, boundaryIdx))
                return boundaryFaceIndex_[boundaryIdx];
        }
        return -1;
    }

    /*!
     * \brief Computes and returns the label of a given throat
     *
     * \param element The element (throat)
     */
    int throatLabel(const std::array<int, 2>& poreLabels) const
    {
        if (poreLabels[0] == poreLabels[1]) // both vertices are inside the domain or on the same boundary face
            return poreLabels[0];
        if (poreLabels[0] == -1) // vertex1 is inside the domain, vertex2 is on a boundary face
            return poreLabels[1];
        if (poreLabels[1] == -1) // vertex2 is inside the domain, vertex1 is on a boundary face
            return poreLabels[0];

        // use the priority list to find out which pore label is favored
        for(const auto i : priorityList_)
        {
            if (poreLabels[0] == boundaryFaceIndex_[i])
                return poreLabels[0];
            if (poreLabels[1] == boundaryFaceIndex_[i])
                return poreLabels[1];
        }

        DUNE_THROW(Dune::InvalidStateException, "Something went wrong with the throat labels");
    }

    /*!
     * \brief Assign parameters for generically created grids
     */
    template <class SetParameter, class GetParameter>
    void assignParameters(const SetParameter& setParameter,
                          const GetParameter& getParameter,
                          const std::size_t numSubregions)
    {
        using cytpe = typename GlobalPosition::value_type;
        using InternalBoundingBox = Dune::AxisAlignedCubeGeometry<cytpe, dimWorld, dimWorld>;
        std::vector<InternalBoundingBox> internalBoundingBoxes;

        // divide the network into subregions, if specified
        if (numSubregions > 0)
        {
            // get bounding boxes of subregions
            for (int i = 0; i < numSubregions; ++i)
            {
                auto lowerLeft = getParamFromGroup<GlobalPosition>(paramGroup_, "Grid.Subregion" + std::to_string(i) + ".LowerLeft");
                auto upperRight = getParamFromGroup<GlobalPosition>(paramGroup_, "Grid.Subregion" + std::to_string(i) + ".UpperRight");
                internalBoundingBoxes.emplace_back(std::move(lowerLeft), std::move(upperRight));
            }
        }

        // get the maximum possible pore body radii such that pore bodies do not intersect
        // (requires ThroatRegionId, if specified)
        const std::vector<Scalar> maxPoreRadius = getMaxPoreRadii_(numSubregions, getParameter);
        std::vector<bool> poreRadiusLimited(gridView_.size(dim), false);

        // get a helper function for getting the pore radius of a pore body not belonging to a subregion
        auto defaultPoreRadius = poreRadiusHelper_(-1);

        // get helper functions for pore body radii on subregions
        std::vector<decltype(defaultPoreRadius)> subregionPoreRadius;
        for (int i = 0; i < numSubregions; ++i)
            subregionPoreRadius.emplace_back(poreRadiusHelper_(i));

        // get helper function for pore volume
        const auto poreVolume = poreVolumeHelper_(getParameter);

        // treat the pore body parameters (label, radius and maybe regionId)
        for (const auto& vertex : vertices(gridView_))
        {
            const auto& pos = vertex.geometry().center();
            const auto vIdxGlobal = gridView_.indexSet().index(vertex);
            const auto poreLabel = boundaryFaceMarkerAtPos(pos);
            setParameter(vertex, "PoreLabel", poreLabel);

            // sets the minimum of the given value and the maximum possible pore body radius
            // and keeps track of capped radii
            auto setRadiusAndLogIfCapped = [&](const Scalar value)
            {
                if (value > maxPoreRadius[vIdxGlobal])
                {
                    poreRadiusLimited[vIdxGlobal] = true;
                    setParameter(vertex, "PoreInscribedRadius", maxPoreRadius[vIdxGlobal]);
                }
                else
                    setParameter(vertex, "PoreInscribedRadius", value);
            };

            if (numSubregions == 0) // assign radius if no subregions are specified
                setRadiusAndLogIfCapped(defaultPoreRadius(vertex, poreLabel));
            else // assign region ids and radii to vertices if they are within a subregion
            {
                // default value for vertices not belonging to a subregion
                setParameter(vertex, "PoreRegionId", -1);
                setRadiusAndLogIfCapped(defaultPoreRadius(vertex, poreLabel));

                for (int id = 0; id < numSubregions; ++id)
                {
                    const auto& subregion = internalBoundingBoxes[id];
                    if (intersectsPointGeometry(vertex.geometry().center(), subregion))
                    {
                        setParameter(vertex, "PoreRegionId", id);
                        setRadiusAndLogIfCapped(subregionPoreRadius[id](vertex, poreLabel));
                    }
                }
            }

            setParameter(vertex, "PoreVolume", poreVolume(vertex, poreLabel));
        }

        // treat throat parameters
        auto defaultThroatRadius = throatRadiusHelper_(-1, getParameter);
        auto defaultThroatLength = throatLengthHelper_(-1, getParameter);

        // get helper functions for throat radii and lengths on subregions
        std::vector<decltype(defaultThroatRadius)> subregionThroatRadius;
        std::vector<decltype(defaultThroatLength)> subregionThroatLength;
        for (int i = 0; i < numSubregions; ++i)
        {
            subregionThroatRadius.emplace_back(throatRadiusHelper_(i, getParameter));
            subregionThroatLength.emplace_back(throatLengthHelper_(i, getParameter));
        }

        // set throat parameters
        for (const auto& element : elements(gridView_))
        {
            if (numSubregions == 0) // assign values if no subregions are specified
            {
                setParameter(element, "ThroatRadius", defaultThroatRadius(element));
                setParameter(element, "ThroatLength", defaultThroatLength(element));
            }
            else // assign values to throats if they are within a subregion
            {
                // default value for elements not belonging to a subregion
                setParameter(element, "ThroatRegionId", -1);
                setParameter(element, "ThroatRadius", defaultThroatRadius(element));
                setParameter(element, "ThroatLength", defaultThroatLength(element));

                for (int id = 0; id < numSubregions; ++id)
                {
                    const auto& subregion = internalBoundingBoxes[id];
                    if (intersectsPointGeometry(element.geometry().center(), subregion))
                    {
                        setParameter(element, "ThroatRegionId", id);
                        setParameter(element, "ThroatRadius", subregionThroatRadius[id](element));
                        setParameter(element, "ThroatLength", subregionThroatLength[id](element));
                    }
                }
            }

            // set the throat label
            const auto vertex0 = element.template subEntity<dim>(0);
            const auto vertex1 = element.template subEntity<dim>(1);
            const std::array poreLabels{static_cast<int>(getParameter(vertex0, "PoreLabel")),
                                        static_cast<int>(getParameter(vertex1, "PoreLabel"))};
            setParameter(element, "ThroatLabel", throatLabel(poreLabels));
        }

        const auto numPoreRadiusLimited = std::count(poreRadiusLimited.begin(), poreRadiusLimited.end(), true);
        if (numPoreRadiusLimited > 0)
            std::cout << "*******\nWarning!  " << numPoreRadiusLimited << " out of " << poreRadiusLimited.size()
            << " pore body radii have been capped automatically in order to prevent intersecting pores\n*******" << std::endl;
    }

private:

    /*!
     * \brief Returns a list of boundary face priorities from user specified input or default values if no input is given
     *
     * This essentially determines the index of a node on an edge or corner corner. For instance, a list of {0,1,2} will give highest priority
     * to the "x"-faces and lowest to the "z-faces".
     */
    BoundaryList getPriorityList_() const
    {
        const auto list = [&]()
        {
            BoundaryList priorityList;
            std::iota(priorityList.begin(), priorityList.end(), 0);

            if (hasParamInGroup(paramGroup_, "Grid.PriorityList"))
            {
                try {
                    // priorities can also be set in the input file
                    priorityList = getParamFromGroup<BoundaryList>(paramGroup_, "Grid.PriorityList");
                }
                // make sure that a priority for each direction is set
                catch(Dune::RangeError& e) {
                    DUNE_THROW(Dumux::ParameterException, "You must specifiy priorities for all directions (" << dimWorld << ") \n" << e.what());
                }
                // make sure each direction is only set once
                if (!isUnique_(priorityList))
                    DUNE_THROW(Dumux::ParameterException, "You must specifiy priorities for all directions (duplicate directions)");

                //make sure that the directions are correct (ranging from 0 to dimWorld-1)
                if (std::any_of(priorityList.begin(), priorityList.end(), []( const int i ){ return (i < 0 || i >= 2*dimWorld); }))
                    DUNE_THROW(Dumux::ParameterException, "You must specifiy priorities for correct directions (0-" << 2*(dimWorld-1) << ")");
            }
            return priorityList;
        }();
        return list;
    }

    void computeBoundingBox_()
    {
        // calculate the bounding box of the local partition of the grid view
        for (const auto& vertex : vertices(gridView_))
        {
            for (int i = 0; i < dimWorld; i++)
            {
                using std::min;
                using std::max;
                bBoxMin_[i] = min(bBoxMin_[i], vertex.geometry().corner(0)[i]);
                bBoxMax_[i] = max(bBoxMax_[i], vertex.geometry().corner(0)[i]);
            }
        }
    }

    /*!
     * \brief Returns a list of boundary face indices from user specified input or default values if no input is given
     */
    BoundaryList getBoundaryFacemarkerInput_() const
    {
        BoundaryList boundaryFaceMarker;
        std::fill(boundaryFaceMarker.begin(), boundaryFaceMarker.end(), 0);
        boundaryFaceMarker[0] = 1;
        boundaryFaceMarker[1] = 1;

        if (hasParamInGroup(paramGroup_, "Grid.BoundaryFaceMarker"))
        {
            try {
                boundaryFaceMarker = getParamFromGroup<BoundaryList>(paramGroup_, "Grid.BoundaryFaceMarker");
            }
            catch (Dune::RangeError& e) {
                DUNE_THROW(Dumux::ParameterException, "You must specifiy all boundaries faces: xmin xmax ymin ymax (zmin zmax). \n" << e.what());
            }
            if (std::none_of(boundaryFaceMarker.begin(), boundaryFaceMarker.end(), []( const int i ){ return i == 1; }))
                DUNE_THROW(Dumux::ParameterException, "At least one face must have index 1");
            if (std::any_of(boundaryFaceMarker.begin(), boundaryFaceMarker.end(), []( const int i ){ return (i < 0 || i > 2*dimWorld); }))
                DUNE_THROW(Dumux::ParameterException, "Face indices must range from 0 to " << 2*dimWorld );
        }
        return boundaryFaceMarker;
    }

    // returns the maximum possible pore body radii such that pore bodies do not intersect
    template <class GetParameter>
    std::vector<Scalar> getMaxPoreRadii_(std::size_t numSubregions, const GetParameter& getParameter) const
    {
        const auto numVertices = gridView_.size(dim);
        std::vector<Scalar> maxPoreRadius(numVertices, std::numeric_limits<Scalar>::max());

        if (!getParamFromGroup<bool>(paramGroup_, "Grid.CapPoreRadii", true))
            return maxPoreRadius;

        // check for a user-specified fixed throat length
        const Scalar inputThroatLength = getParamFromGroup<Scalar>(paramGroup_, "Grid.ThroatLength", -1.0);
        std::vector<Scalar> subregionInputThroatLengths;

        if (numSubregions > 0)
        {
            for (int i = 0; i < numSubregions; ++i)
            {
                // adapt the parameter group if there are subregions
                const std::string paramGroup = paramGroup_ + ".SubRegion" + std::to_string(i);
                const Scalar subregionInputThroatLength = getParamFromGroup<Scalar>(paramGroup, "Grid.ThroatLength", -1.0);
                subregionInputThroatLengths.push_back(subregionInputThroatLength);
            }
        }

        for (const auto& element : elements(gridView_))
        {
            // do not cap the pore radius if a user-specified throat length if given
            if (numSubregions > 0)
            {
                // check subregions
                const auto subregionId = getParameter(element, "ThroatRegionId");
                if (subregionId >= 0)
                {
                    // throat lies within a subregion
                    if (subregionInputThroatLengths[subregionId] > 0.0)
                        continue;
                }
                else if (inputThroatLength > 0.0) // throat does not lie within a subregion
                    continue;
            }
            else if (inputThroatLength > 0.0) // no subregions
                continue;

            // No fixed throat lengths given, check for max. pore radius.
            // We define this as 1/2 of the length (minus a user specified value) of the shortest pore throat attached to the pore body.
            const Scalar delta = element.geometry().volume();
            static const Scalar minThroatLength = getParamFromGroup<Scalar>(paramGroup_, "Grid.MinThroatLength", 1e-6);
            const Scalar maxRadius = (delta - minThroatLength)/2.0;
            for (int vIdxLocal = 0; vIdxLocal < 2 ; ++vIdxLocal)
            {
                const int vIdxGlobal = gridView_.indexSet().subIndex(element, vIdxLocal, dim);
                maxPoreRadius[vIdxGlobal] = std::min(maxPoreRadius[vIdxGlobal], maxRadius);
            }
        }

        return maxPoreRadius;
    }

    // returns a lambda taking a vertex and returning a radius
    auto poreRadiusHelper_(const int subregionId) const
    {
        // adapt the parameter name if there are subregions
        const std::string prefix = subregionId < 0 ? "Grid." : "Grid.Subregion" + std::to_string(subregionId) + ".";

        // prepare random number generation for lognormal parameter distribution
        std::mt19937 generator;

        // lognormal random number distribution
        std::lognormal_distribution<> poreRadiusDist;

        const Scalar fixedPoreRadius = getParamFromGroup<Scalar>(paramGroup_, prefix + "PoreRadius", -1);
        if (fixedPoreRadius < 0.0)
        {
            const auto type = getParamFromGroup<std::string>(paramGroup_, prefix + "ParameterType");
            if (type != "lognormal")
                DUNE_THROW(Dune::InvalidStateException, "Unknown parameter type " << type);

            // allow to specify a seed to get reproducible results
            const auto seed = getParamFromGroup<unsigned int>(paramGroup_, prefix + "ParameterRandomNumberSeed", std::random_device{}());
            generator.seed(seed);

            // if we use a distribution, get the mean and standard deviation from input file
            const Scalar meanPoreRadius = getParamFromGroup<Scalar>(paramGroup_, prefix + "MeanPoreRadius");
            const Scalar stddevPoreRadius = getParamFromGroup<Scalar>(paramGroup_, prefix + "StandardDeviationPoreRadius");
            const Scalar variance = stddevPoreRadius*stddevPoreRadius;

            using std::log;
            using std::sqrt;
            const Scalar mu = log(meanPoreRadius/sqrt(1.0 + variance/(meanPoreRadius*meanPoreRadius)));
            const Scalar sigma = sqrt(log(1.0 + variance/(meanPoreRadius*meanPoreRadius)));

            std::lognormal_distribution<>::param_type params(mu, sigma);
            poreRadiusDist.param(params);
        }

        // check if pores for certain labels should be treated in a special way
        const auto poreLabelsToSetFixedRadius = getParamFromGroup<std::vector<int>>(paramGroup_, prefix + "PoreLabelsToSetFixedRadius", std::vector<int>{});
        const auto poreLabelsToApplyFactorForRadius = getParamFromGroup<std::vector<int>>(paramGroup_, prefix + "PoreLabelsToApplyFactorForRadius", std::vector<int>{});
        const auto poreRadiusForLabel = getParamFromGroup<std::vector<Scalar>>(paramGroup_, prefix + "FixedPoreRadiusForLabel", std::vector<Scalar>{});
        const auto poreRadiusFactorForLabel = getParamFromGroup<std::vector<Scalar>>(paramGroup_, prefix + "PoreRadiusFactorForLabel", std::vector<Scalar>{});

        auto maybeModifiedRadius = [&, fixedPoreRadius, poreRadiusDist, generator,
                                    poreLabelsToSetFixedRadius, poreLabelsToApplyFactorForRadius,
                                    poreRadiusForLabel, poreRadiusFactorForLabel](const auto& vertex, const int poreLabel) mutable
        {
            // default radius to return: either a fixed (user-specified) one or a randomly drawn one
            auto radius = [&]() { return fixedPoreRadius > 0.0 ? fixedPoreRadius : poreRadiusDist(generator); };

            // check if pores for certain labels should be treated in a special way
            if (poreLabelsToSetFixedRadius.empty() && poreLabelsToApplyFactorForRadius.empty())
                return radius(); // nothing special to be done

            // set a fixed radius for a given label
            if (!poreLabelsToSetFixedRadius.empty() || !poreRadiusForLabel.empty())
            {
                if (poreLabelsToSetFixedRadius.size() != poreRadiusForLabel.size())
                    DUNE_THROW(Dumux::ParameterException, "PoreLabelsToSetFixedRadius must be of same size as FixedPoreRadiusForLabel");

                if (const auto it = std::find(poreLabelsToSetFixedRadius.begin(), poreLabelsToSetFixedRadius.end(), poreLabel); it != poreLabelsToSetFixedRadius.end())
                    return poreRadiusForLabel[std::distance(poreLabelsToSetFixedRadius.begin(), it)];
            }

            // multiply the pore radius by a given value for a given label
            if (!poreLabelsToApplyFactorForRadius.empty() || !poreRadiusFactorForLabel.empty())
            {
                if (poreLabelsToApplyFactorForRadius.size() != poreRadiusFactorForLabel.size())
                    DUNE_THROW(Dumux::ParameterException, "PoreLabelsToApplyFactorForRadius must be of same size as PoreRadiusFactorForLabel");

                if (const auto it = std::find(poreLabelsToApplyFactorForRadius.begin(), poreLabelsToApplyFactorForRadius.end(), poreLabel); it != poreLabelsToApplyFactorForRadius.end())
                    return poreRadiusFactorForLabel[std::distance(poreLabelsToApplyFactorForRadius.begin(), it)] * radius();
            }

            // default
            return radius();
        };

        return maybeModifiedRadius;
    }

    template <class GetParameter>
    auto poreVolumeHelper_(const GetParameter& getParameter) const
    {
        // automatically determine the pore volume if not provided by the grid file
        const auto poreVolume = [&] (const auto& vertex, const auto vIdx)
        {
            static const auto geometry = Pore::shapeFromString(getParamFromGroup<std::string>(paramGroup_, "Grid.PoreGeometry"));
            const Scalar r = getParameter(vertex, "PoreInscribedRadius");
            Scalar volume = 0.0;

            if (geometry == Pore::Shape::cylinder)
            {
                static const Scalar fixedHeight = getParamFromGroup<Scalar>(paramGroup_, "Grid.PoreHeight", -1.0);
                const Scalar h = fixedHeight > 0.0 ? fixedHeight : getParameter(vertex, "PoreHeight");
                volume = Pore::volume(geometry, r, h);
            }
            else
                volume = Pore::volume(geometry, r);

            static const auto capPoresOnBoundaries = getParamFromGroup<std::vector<int>>(paramGroup_, "Grid.CapPoresOnBoundaries", std::vector<int>{});
            if (!isUnique_(capPoresOnBoundaries))
                DUNE_THROW(Dune::InvalidStateException, "CapPoresOnBoundaries must not contain duplicates");

            if (capPoresOnBoundaries.empty())
                return volume;
            else
            {
                std::size_t numCaps = 0;
                Scalar factor = 1.0;
                const auto& pos = vertex.geometry().center();
                for (auto boundaryIdx : capPoresOnBoundaries)
                {
                    if (onBoundary_(pos, boundaryIdx))
                    {
                        factor *= 0.5;
                        ++numCaps;
                    }
                }

                if (numCaps > dimWorld)
                    DUNE_THROW(Dune::InvalidStateException, "Pore " << vIdx << " at " << pos << " capped " << numCaps << " times. Capping should not happen more than " << dimWorld << " times");

                return volume * factor;
            }
        };

        return poreVolume;
    }

    // returns a lambda taking a element and returning a radius
    template <class GetParameter>
    auto throatRadiusHelper_(const int subregionId, const GetParameter& getParameter) const
    {
        // adapt the parameter name if there are subregions
        const std::string prefix = subregionId < 0 ? "Grid." : "Grid.Subregion" + std::to_string(subregionId) + ".";

        // check for a user-specified fixed throat radius
        const Scalar inputThroatRadius = getParamFromGroup<Scalar>(paramGroup_, prefix + "ThroatRadius", -1.0);

        // shape parameter for calculation of throat radius
        const Scalar throatN = getParamFromGroup<Scalar>(paramGroup_, prefix + "ThroatRadiusN", 0.1);

        auto getThroatRadius = [&, inputThroatRadius, throatN](const Element& element)
        {
            const Scalar delta = element.geometry().volume();
            typedef typename GridView::template Codim<dim>::Entity PNMVertex;
            const std::array<PNMVertex, 2> vertices = {element.template subEntity<dim>(0), element.template subEntity<dim>(1)};

            // the element parameters (throat radius and length)
            if (inputThroatRadius > 0.0)
                return inputThroatRadius;
            else
            {
                const Scalar poreRadius0 = getParameter(vertices[0], "PoreInscribedRadius");
                const Scalar poreRadius1 = getParameter(vertices[1], "PoreInscribedRadius");
                return Throat::averagedRadius(poreRadius0, poreRadius1, delta, throatN);
            }
        };

        return getThroatRadius;
    }

    // returns a lambda taking a element and returning a length
    template <class GetParameter>
    auto throatLengthHelper_(int subregionId, const GetParameter& getParameter) const
    {
        auto getThroatLength = [&, subregionId](const Element& element)
        {
            // adapt the parameter name if there are subregions
            const std::string prefix = subregionId < 0 ? "Grid." : "Grid.Subregion" + std::to_string(subregionId) + ".";

            static const Scalar inputThroatLength = getParamFromGroup<Scalar>(paramGroup_, prefix + "ThroatLength", -1.0);
            if (inputThroatLength > 0.0)
                return inputThroatLength;

            const Scalar delta = element.geometry().volume();

            // decide whether to substract the pore radii from the throat length or not
            static const bool substractRadiiFromThroatLength = getParamFromGroup<bool>(paramGroup_, prefix + "SubstractRadiiFromThroatLength", true);

            if (substractRadiiFromThroatLength)
            {
                typedef typename GridView::template Codim<dim>::Entity PNMVertex;
                const std::array<PNMVertex, 2> vertices = {element.template subEntity<dim>(0), element.template subEntity<dim>(1)};
                const Scalar result = delta - getParameter(vertices[0], "PoreInscribedRadius") - getParameter(vertices[1], "PoreInscribedRadius");
                if (result <= 0.0)
                    DUNE_THROW(Dune::GridError, "Pore radii are so large they intersect! Something went wrong at element " << gridView_.indexSet().index(element));
                else
                    return result;
            }
            else
                return delta;
        };

        return getThroatLength;
    }

    bool onBoundary_(const GlobalPosition& pos, std::size_t boundaryIdx) const
    {
        constexpr auto eps = 1e-8; //TODO

        static constexpr std::array boundaryIdxToCoordinateIdx{0, 0, 1, 1, 2, 2};
        const auto coordinateIdx = boundaryIdxToCoordinateIdx[boundaryIdx];
        auto isMaxBoundary = [] (int n) { return (n % 2 != 0); };

        if (isMaxBoundary(boundaryIdx))
            return pos[coordinateIdx] > bBoxMax_[coordinateIdx] - eps;
        else
            return pos[coordinateIdx] < bBoxMin_[coordinateIdx] + eps;
    }

    // check if a container is unique
    // copy the container such that is does not get altered by std::sort
    template <class T>
    bool isUnique_(T t) const
    {
        std::sort(t.begin(), t.end());
        return (std::unique(t.begin(), t.end()) == t.end());
    }

    const GridView gridView_;

    const std::string paramGroup_;
    BoundaryList priorityList_;
    BoundaryList boundaryFaceIndex_;

    //! the bounding box of the whole domain
    GlobalPosition bBoxMin_ = GlobalPosition(std::numeric_limits<double>::max());
    GlobalPosition bBoxMax_ = GlobalPosition(std::numeric_limits<double>::min());
};

} // namespace Dumux::PoreNetwork

#endif
