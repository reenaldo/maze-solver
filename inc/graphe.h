#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/* Lightweight adjacency-list graph representation for unweighted graphs.
 * - AdjList stores a dynamic array of neighbor indices.
 * - Graph holds an array of AdjList, the number of vertices (order),
 *   and whether edges are oriented (directed).
 */

/* ===== Structures ===== */

typedef struct adj_list {
    size_t nb_neighbors;   
    size_t capacity;      
    size_t *neighbors;     
} AdjList;

typedef struct graph {
    bool oriented;         
    size_t order;          
    size_t nb_edges;       
    AdjList *list;         
} Graph;

/* ===== Fonctions ===== */

/* Create an empty graph with `order` vertices. `oriented` selects directedness. */
Graph* create_graph(size_t order, bool oriented);
/* Free all memory used by the graph. */
void free_graph(Graph *g);
/* Add an edge u->v (or undirected edge if graph is non-oriented). */
void add_edge(Graph *g, size_t u, size_t v);
/* Remove an edge u->v if present. */
void remove_edge(Graph *g, size_t u, size_t v);
/* Check whether an edge u->v exists. */
bool edge_exists(const Graph *g, size_t u, size_t v);
/* Print a simple textual representation of the graph to stdout. */
void print_graph(const Graph *g);


// ===== Helpers pour tests & validation =====
struct graph; 

/* Create a w×h grid graph (undirected) where each cell is connected to
 * its orthogonal neighbors. Useful for maze/grid tests. */
Graph* create_grid_graph(size_t width, size_t height);

/* Check whole-graph connectivity using BFS; returns true if the graph
 * is connected (single component). */
bool is_graph_connected_bfs(const Graph *g);

#endif 

