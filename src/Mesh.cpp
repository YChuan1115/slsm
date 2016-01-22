/*
  Copyright (c) 2015-2016 Lester Hedges <lester.hedges+lsm@gmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Mesh.h"

/*! \file Mesh.cpp
    \brief A class for the fixed grid, finite-element mesh.
 */

namespace lsm
{
    Mesh::Mesh(unsigned int width_,
            unsigned int height_,
            bool isPeriodic_) :

            width(width_),
            height(height_),
            nElements(width*height),
            nNodes((1+width)*(1+height)),
            isPeriodic(isPeriodic_)
    {
        // Resize element and node data structures.
        elements.resize(nElements);
        nodes.resize(nNodes);

        // Resize 2D to 1D mapping vector.
        xyToIndex.resize(width+1);
        for (unsigned int i=0;i<width+1;i++)
            xyToIndex[i].resize(height+1);

        // Calculate node nearest neighbours.
        initialiseNodes();

        // Initialise elements (and node to element connectivity).
        initialiseElements();
    }

    unsigned int Mesh::getClosestNode(const Coord& point)
    {
        // Get element index.
        unsigned int element = getElement(point);

        // Work out x and y position within the element (remainder).
        double dx = point.x - std::floor(point.x);
        double dy = point.y - std::floor(point.y);

        // Point lies in left half.
        if (dx < 0.5)
        {
            // Lower left quadrant.
            if (dy < 0.5) return elements[element].nodes[0];

            // Upper left quadrant.
            else return elements[element].nodes[3];
        }

        // Point lies in right half.
        else
        {
            // Lower right quadrant.
            if (dy < 0.5) return elements[element].nodes[1];

            // Upper right quadrant.
            else return elements[element].nodes[2];
        }
    }

    unsigned int Mesh::getClosestNode(double x, double y)
    {
        // Get element index.
        unsigned int element = getElement(x, y);

        // Work out x and y position within the element (remainder).
        double dx = x - std::floor(x);
        double dy = y - std::floor(y);

        // Point lies in left half.
        if (dx < 0.5)
        {
            // Lower left quadrant.
            if (dy < 0.5) return elements[element].nodes[0];

            // Upper left quadrant.
            else return elements[element].nodes[3];
        }

        // Point lies in right half.
        else
        {
            // Lower right quadrant.
            if (dy < 0.5) return elements[element].nodes[1];

            // Upper right quadrant.
            else return elements[element].nodes[2];
        }
    }

    unsigned int Mesh::getElement(const Coord& point)
    {
        // Work out x and y element indices (cells are unit width).
        unsigned int elementX = std::floor(point.x);
        unsigned int elementY = std::floor(point.y);

        // Return global element index.
        return (elementY*width + elementX);
    }

    unsigned int Mesh::getElement(double x, double y)
    {
        // Work out x and y element indices (cells are unit width).
        unsigned int elementX = std::floor(x);
        unsigned int elementY = std::floor(y);

        // Return global element index.
        return (elementY*width + elementX);
    }

    void Mesh::initialiseNodes()
    {
        // Coordinates of the node.
        unsigned int x, y;

        // Loop over all nodes.
        for (unsigned int i=0;i<nNodes;i++)
        {
            // Mark node as in bulk.
            nodes[i].isDomain = false;

            // Zero number of connected elements.
            nodes[i].nElements = 0;

            // Work out node coordinates.
            x = i % (width + 1);
            y = int(i / (width + 1));

            // Node lies on the domain boundary.
            if ((x == 0) || (x == width) || (y == 0) || (y == height))
                nodes[i].isDomain = true;

            // Set node coordinates.
            nodes[i].coord.x = x;
            nodes[i].coord.y = y;

            // Add to 2D mapping vector.
            xyToIndex[x][y] = i;

            // Determine nearest neighbours.
            initialiseNeighbours(i, x, y);
        }
    }

    void Mesh::initialiseElements()
    {
        // Coordinates of the element.
        unsigned int x, y;

        // Number of nodes along width of mesh (number of elements plus one)
        unsigned int w = width + 1;

        // Relative node coordinates of the element.
        Coord nodeCoords[4] = {{-1, -1}, {1., -1}, {1, 1}, {-1, 1}};

        double invSqrtThree = 1.0 / sqrt(3.0);

        // Loop over all elements.
        for (unsigned int i=0;i<nElements;i++)
        {
            // Work out element coordinates.
            x = i % width;
            y = int(i / width);

            // Store coordinates of elemente centre.
            elements[i].coord.x = x + 0.5;
            elements[i].coord.y = y + 0.5;

            // Store Guass point coordinates.
            for (unsigned int j=0;j<4;j++)
            {
                elements[i].gaussPoints[j].x = elements[i].coord.x + (invSqrtThree * 0.5 * nodeCoords[j].x);
                elements[i].gaussPoints[j].y = elements[i].coord.y + (invSqrtThree * 0.5 * nodeCoords[j].y);
            }

            // Store connectivity (element --> node)

            // Node on bottom left corner of element.
            elements[i].nodes[0] = x + (y * w);

            // Node on bottom right corner of element.
            elements[i].nodes[1] = x + 1 + (y * w);

            // Node on top right corner of element.
            elements[i].nodes[2] = x + 1 + ((y + 1) * w);

            // Node on top right corner of element.
            elements[i].nodes[3] = x + ((y + 1) * w);

            // Fill reverse connectivity arrays (node --> element)
            for (unsigned int j=0;j<4;j++)
            {
                unsigned int node = elements[i].nodes[j];
                nodes[node].elements[nodes[node].nElements] = i;
                nodes[node].nElements++;
            }
        }
    }

    void Mesh::initialiseNeighbours(unsigned int node, unsigned int x, unsigned int y)
    {
        // Number of nodes along width of mesh (number of elements plus one)
        unsigned int w = width + 1;

        // Neighbours to left and right.
        nodes[node].neighbours[0] = (x - 1 + w) % w + (y * w);
        nodes[node].neighbours[1] = (x + 1 + w) % w + (y * w);

        // Neighbours below and above.
        nodes[node].neighbours[2] = x + (w * ((y - 1 + w) % w));
        nodes[node].neighbours[3] = x + (w * ((y + 1 + w) % w));

        // The mesh isn't periodic, flag out of bounds neigbours.
        if (!isPeriodic)
        {
            // Node is on first or last row.
            if (x == 0) nodes[node].neighbours[0] = nNodes;
            else if (x == width) nodes[node].neighbours[1] = nNodes;

            // Node is on first or last column.
            if (y == 0) nodes[node].neighbours[2] = nNodes;
            else if (y == height) nodes[node].neighbours[3] = nNodes;
        }
    }
}
