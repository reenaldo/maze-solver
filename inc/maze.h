#ifndef MAZE_H
#define MAZE_H

#include "graphe.h"
#include <time.h>

/* Maze generation helpers built on top of the Graph adjacency-list.
 * Functions below treat a Graph as a width×height grid maze where each
 * vertex represents a cell. Coordinates are mapped to vertex ids via
 * `cell_id`.
 */

/* Convert (x,y) grid coordinates to the corresponding vertex id. */
size_t cell_id(size_t x, size_t y, size_t width);

/* Generate a new maze as a Graph for a width×height grid. */
Graph* generate_maze(size_t width, size_t height);

/* Recursive division step used by the maze generator. Operates on the
 * sub-rectangle [x0,y0]..[x1,y1] within the grid. */
void divide(Graph *g, size_t x0, size_t y0, size_t x1, size_t y1, size_t width, size_t height);
#endif

