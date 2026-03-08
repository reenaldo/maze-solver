#pragma once
#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include "graphe.h"

/* Short BFS utilities for unweighted graphs.
 *
 * These helpers compute BFS distances/parents up to a goal and
 * reconstruct a path from a parent array produced by BFS.
 */

/* Run BFS to compute distances and parent pointers (until goal found).
 * g      - input graph
 * goal   - target node index
 * dist   - output array of distances (one entry per vertex)
 * parent - output array of parent indices (one entry per vertex)
 * Returns true on successful computation (goal reachable or search completed).
 */
bool bfs_precompute_to_goal(const Graph *g, size_t goal, size_t *dist, size_t *parent);

/* Reconstruct the path from start to goal using the BFS parent array.
 * g        - input graph (used for bounds/validations)
 * start    - start vertex index
 * goal     - goal vertex index
 * parent   - parent array produced by BFS
 * path_out - buffer to receive the sequence of vertex indices (start..goal)
 * Returns the number of vertices written to path_out, or -1 on error.
 */
ssize_t bfs_reconstruct_path(const Graph *g, size_t start, size_t goal,
                             const size_t *parent, size_t *path_out);
                             
