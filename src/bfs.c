#include "../inc/bfs.h"
#include <stddef.h>   
#include <stdint.h>   
#include <stdbool.h>  
#include <stdlib.h>   

/*
 * Compute distances and parent pointers from every node to the given goal
 * (BFS started from goal). Returns true on success, false on error.
 */
bool bfs_precompute_to_goal(const Graph *g, size_t goal, size_t *dist, size_t *parent) {
    if (!g || goal >= g->order || !dist || !parent) return false;

    /* disallow self-loops in the graph (defensive check) */
    for (size_t u = 0; u < g->order; u++) {
        const AdjList au = g->list[u];
        for (size_t i = 0; i < au.nb_neighbors; i++) {
            if (au.neighbors[i] == u) {
                return false;
            }
        }
    }

    size_t n = g->order;
    bool   *vis = (bool*)calloc(n, sizeof(bool));
    size_t *q   = (size_t*)malloc(n * sizeof(size_t));
    if (!vis || !q) { free(vis); free(q); return false; }

    /* initialize distance/parent arrays */
    for (size_t i = 0; i < n; i++) {
        dist[i]   = SIZE_MAX;
        parent[i] = SIZE_MAX;
    }

    size_t head = 0, tail = 0;

    /* start BFS from goal so dist[x] = distance from x to goal */
    vis[goal]    = true;
    dist[goal]   = 0;
    parent[goal] = goal;
    q[tail++]    = goal;

    /* standard FIFO BFS loop */
    while (head < tail) {
        size_t u = q[head++];
        const AdjList au = g->list[u];
        for (size_t i = 0; i < au.nb_neighbors; i++) {
            size_t v = au.neighbors[i];
            if (!vis[v]) {
                vis[v]    = true;
                dist[v]   = dist[u] + 1;
                parent[v] = u;              
                q[tail++] = v;
            }
        }
    }

    /* cleanup temporary storage */
    free(vis);
    free(q);
    return true;
}

/*
 * Reconstruct a path from start to goal using the parent pointers produced
 * by bfs_precompute_to_goal. Returns length of path on success, -1 on error.
 */
ssize_t bfs_reconstruct_path(const Graph *g, size_t start, size_t goal,
                             const size_t *parent, size_t *path_out) {
    /* validate inputs and parent array */
    if (!g || !parent || !path_out || start >= g->order || goal >= g->order) return -1;
    if (parent[start] == SIZE_MAX) return -1;  
    if (parent[goal] != goal) return -1;     

    size_t len = 0, cur = start;

    /* follow parent pointers until we reach the goal/root or exceed bounds */
    for (size_t steps = 0; steps < g->order; steps++) {
        path_out[len++] = cur;
        if (cur == parent[cur]) break;  /* reached root (goal) */
        cur = parent[cur];
    }

    /* confirm the reconstructed path actually ends at the goal */
    if (path_out[len-1] != goal) {
        return -1;
    }

    return (ssize_t)len;
}

