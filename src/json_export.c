#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "../inc/json_export.h"


/* Convert linear cell id to (x,y) coordinates given row width. */
static void id_to_xy(size_t id, size_t width, size_t *x, size_t *y) {
    *x = id % width;
    *y = id / width;
}

int export_maze_bfs_json(const Graph *g, size_t width, size_t height,
                         size_t start, size_t goal, const char *filename) {
    if (!g || !filename) return 1;

    size_t n = g->order;

    bool *seen = calloc(n, sizeof(bool));
    if (!seen) return 2;
    size_t *queue = malloc(n * sizeof(size_t));
    size_t *parent = malloc(n * sizeof(size_t));
    size_t qh = 0, qt = 0;

    size_t steps_cap = n ? n : 1;
    size_t steps_len = 0;
    size_t *steps = malloc(steps_cap * sizeof(size_t));

    if (!queue || !parent || !steps) {
        free(seen);
        free(queue);
        free(parent);
        free(steps);
        return 3;
    }

    for (size_t i = 0; i < n; i++) parent[i] = SIZE_MAX;

    queue[qt++] = start; seen[start] = true; parent[start] = start;

    while (qh < qt) {
        size_t v = queue[qh++];

        if (steps_len >= steps_cap) {
            steps_cap *= 2;
            size_t *tmp = realloc(steps, steps_cap * sizeof(size_t));
            if (!tmp) break;
            steps = tmp;
        }
        steps[steps_len++] = v;

        if (v == goal) {
            break;
        }

        AdjList al = g->list[v];
        for (size_t i = 0; i < al.nb_neighbors; i++) {
            size_t u = al.neighbors[i];
            if (!seen[u]) {
                seen[u] = true;
                parent[u] = v;
                queue[qt++] = u;
            }
        }
    }

    size_t *path = NULL;
    size_t path_len = 0;
    if (parent[goal] != SIZE_MAX) {
        path = malloc(n * sizeof(size_t));
        if (path) {
            size_t cur = goal;
            while (1) {
                path[path_len++] = cur;
                if (cur == parent[cur]) break; 
                cur = parent[cur];
            }
            for (size_t i = 0; i < path_len/2; i++) {
                size_t tmp = path[i]; path[i] = path[path_len-1-i]; path[path_len-1-i] = tmp;
            }
        }
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        free(seen); free(queue); free(parent); free(steps); free(path);
        return 4;
    }

    fprintf(f, "{\n");

    /* Emit maze cells with wall booleans */
    fprintf(f, "  \"maze\": [\n");
    for (size_t y = 0; y < height; y++) {
        fprintf(f, "    [\n");
        for (size_t x = 0; x < width; x++) {
            size_t id = y * width + x;
            bool up = true, down = true, left = true, right = true;

            if (y > 0) {
                if (edge_exists(g, id, id - width)) up = false;
            }
            if (y + 1 < height) {
                if (edge_exists(g, id, id + width)) down = false;
            }
            if (x > 0) {
                if (edge_exists(g, id, id - 1)) left = false;
            }
            if (x + 1 < width) {
                if (edge_exists(g, id, id + 1)) right = false;
            }
            fprintf(f, "      {\"x\":%zu,\"y\":%zu,\"up\":%s,\"down\":%s,\"left\":%s,\"right\":%s}%s\n",
                    x, y, up?"true":"false", down?"true":"false", left?"true":"false", right?"true":"false",
                    (x + 1 < width) ? "," : "");
        }
        fprintf(f, "    ]%s\n", (y + 1 < height) ? "," : "");
    }
    fprintf(f, "  ],\n");

    size_t sx, sy, gx, gy;
    id_to_xy(start, width, &sx, &sy);
    id_to_xy(goal, width, &gx, &gy);
    fprintf(f, "  \"start\": [%zu, %zu],\n", sx, sy);
    fprintf(f, "  \"end\": [%zu, %zu],\n", gx, gy);

    fprintf(f, "  \"steps\": [\n");
    for (size_t i = 0; i < steps_len; i++) {
        size_t id = steps[i]; size_t x, y; id_to_xy(id, width, &x, &y);
        fprintf(f, "    {\"x\":%zu, \"y\":%zu, \"type\": \"visit\"}%s\n",
                x, y, (i + 1 < steps_len || path_len>0) ? "," : "");
    }

    for (size_t i = 0; i < path_len; i++) {
        size_t id = path[i]; size_t x, y; id_to_xy(id, width, &x, &y);
        fprintf(f, "    {\"x\":%zu, \"y\":%zu, \"type\": \"path\"}%s\n",
                x, y, (i + 1 < path_len) ? "," : "");
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);

    free(seen); free(queue); free(parent); free(steps); free(path);
    return 0;
}

