#pragma once
#include "graphe.h"

/* Export maze data and BFS traversal to a JSON file.
 *
 * Writes the graph `g` interpreted as a width×height maze, with `start` and
 * `goal` vertices, plus BFS traversal/solution steps to `filename`.
 * Returns 0 on success, non-zero on failure (I/O or serialization error).
 */
int export_maze_bfs_json(const Graph *g, size_t width, size_t height,
                         size_t start, size_t goal, const char *filename);
                         
