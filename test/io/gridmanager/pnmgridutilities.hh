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
 * \brief Utility functions for writing pore-network grids to vtp files.
 */
#ifndef DUMUX_TEST_IO_GRIDMANAGER_TEST_PNM_GRID_UTILITIES
#define DUMUX_TEST_IO_GRIDMANAGER_TEST_PNM_GRID_UTILITIES

#include "config.h"

#include <vector>
#include <dune/grid/io/file/vtk.hh>

namespace Dumux {

//! Extract the vertex parameters from a pore network.
template<class GridView, class GridData>
std::vector<std::vector<double>> getVertexParams(const GridView& gridView, const GridData& gridData)
{
    const auto someVertex = *(vertices(gridView).begin());
    const auto numVertexParams = gridData.parameters(someVertex).size();
    std::vector<std::vector<double>> result(numVertexParams);

    for(int i = 0; i < result.size(); ++i)
        result[i].resize(gridView.size(1));

    for(const auto& vertex : vertices(gridView))
    {
        const auto vIdx = gridView.indexSet().index(vertex);
        for(int i = 0; i < result.size(); ++i)
            result[i][vIdx] = gridData.parameters(vertex)[i];
    }

    return result;
}

//! Extract the element parameters from a pore network.
template<class GridView, class GridData>
std::vector<std::vector<double>> getElementParams(const GridView& gridView, const GridData& gridData)
{
    const auto someElement = *(elements(gridView).begin());
    const auto numElementParams = gridData.parameters(someElement).size();
    std::vector<std::vector<double>> result(numElementParams);

    for(int i = 0; i < result.size(); ++i)
        result[i].resize(gridView.size(0));

    for(const auto& element : elements(gridView))
    {
        const auto eIdx = gridView.indexSet().index(element);
        for(int i = 0; i < result.size(); ++i)
            result[i][eIdx] = gridData.parameters(element)[i];
    }

    return result;
}

//! Write a pore-network grid to a vtp file.
template<class GridView, class GridData>
void writeToVtk(const std::string& fileName, const GridView& gridView, const GridData& gridData)
{
    using VTKWriter = Dune::VTKWriter<GridView>;
    auto poreParameterNames = gridData->vertexParameterNames();
    auto throatParameterNames = gridData->elementParameterNames();

    // make first letter of names lower case for consistency
    for (auto& name : poreParameterNames)
        name[0] = std::tolower(name[0]);
    for (auto& name : throatParameterNames)
        name[0] = std::tolower(name[0]);

    const auto vertexData = getVertexParams(gridView, *gridData);
    const auto elementData = getElementParams(gridView, *gridData);

    VTKWriter vtkWriter(gridView);
    for(int i = 0; i < poreParameterNames.size(); ++i)
        vtkWriter.addVertexData(vertexData[i], poreParameterNames[i]);
    for(int i = 0; i < throatParameterNames.size(); ++i)
        vtkWriter.addCellData(elementData[i], throatParameterNames[i]);

    vtkWriter.write(fileName);
}

} // end namespace Dumux

#endif
