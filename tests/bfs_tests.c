#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "../inc/graphe.h"
#include "../inc/bfs.h"
#include "../inc/maze.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

static const char *g_test_id = NULL;
static const char *g_test_desc = NULL;

#define TEST_LINE_WIDTH 100

#define COLOR_GREEN "\x1b[32m"
#define COLOR_RED "\x1b[31m"
#define COLOR_RESET "\x1b[0m"

static void print_test_line_prefix(const char *id, const char *desc) {
    g_test_id = id;
    g_test_desc = desc;
     
     printf("Test %s - %s", id, desc);
     size_t printed = 8 + strlen(id) + strlen(desc); 
    int dots = TEST_LINE_WIDTH - (int)printed - 4; 
    if (dots < 1) dots = 1;
    for (int i = 0; i < dots; ++i) putchar('.');
    fflush(stdout);
}

static void print_test_ok(void) {
    printf(" " COLOR_GREEN "OK" COLOR_RESET "\n");
    fflush(stdout);
    g_test_id = NULL;
    g_test_desc = NULL;
}

static void print_test_fail(const char *reason, int line) {
    printf(" " COLOR_RED "FAIL" COLOR_RESET "\n");
    if (g_test_id) printf("Echec du test %s\n", g_test_id);
    if (reason) printf("  raison: %s (ligne %d)\n", reason, line);
    fflush(stdout);
    exit(EXIT_FAILURE);
}

#define REQUIRE(cond, msg) do { \
  if (!(cond)) { print_test_fail(msg, __LINE__); } \
} while(0)

static double now_monotonic_test(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static Graph* build_grid_graph(size_t w, size_t h) {
    Graph *g = create_graph(w * h, false);
    if (!g) return NULL;
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            size_t id = y * w + x;
            if (x + 1 < w) add_edge(g, id, id + 1);
            if (y + 1 < h) add_edge(g, id, id + w);
        }
    }
    return g;
}

static void test_bfs_precompute_invalid_arguments(void) {
    print_test_line_prefix("1.1", "bfs_precompute_to_goal invalid args");

    size_t n = 4;
    size_t *dist = malloc(n * sizeof(size_t));
    size_t *parent = malloc(n * sizeof(size_t));
    REQUIRE(dist != NULL && parent != NULL, "alloc arrays");

    Graph *g = create_graph(n, false);
    
    REQUIRE(g != NULL, "create_graph for invalid-arg test");
    REQUIRE(!bfs_precompute_to_goal(NULL, 0, dist, parent), "g == NULL should return false");
    REQUIRE(!bfs_precompute_to_goal(g, 0, NULL, parent), "dist == NULL should return false");
    REQUIRE(!bfs_precompute_to_goal(g, 0, dist, NULL), "parent == NULL should return false");

    free(dist);
    free(parent);
    free_graph(g);

    print_test_ok();
}

static void test_bfs_inaccessible_two_components(void) {
    print_test_line_prefix("1.2", "inaccessible: two components, goal in A start in B");

    size_t n = 4;
    size_t *dist = malloc(n * sizeof(size_t));
    size_t *parent = malloc(n * sizeof(size_t));
    size_t *path_out = malloc(n * sizeof(size_t));
    REQUIRE(dist && parent && path_out, "alloc arrays for inaccessible test");

    Graph *g = create_graph(n, false);
    REQUIRE(g != NULL, "create_graph for inaccessible test");
    add_edge(g, 0, 1);
    add_edge(g, 2, 3);

    size_t goal = 0; 
    size_t start = 2; 

    REQUIRE(bfs_precompute_to_goal(g, goal, dist, parent), "precompute should succeed for goal in A");

    REQUIRE(parent[start] == SIZE_MAX, "parent[start] must be SIZE_MAX for unreachable vertex");

    ssize_t len = bfs_reconstruct_path(g, start, goal, parent, path_out);
    REQUIRE(len == -1, "reconstruct should return -1 for unreachable start");

    free(dist);
    free(parent);
    free(path_out);
    free_graph(g);

    print_test_ok();
}

static void test_bfs_unique_path_tree(void) {
    print_test_line_prefix("1.3", "unique path (tree): reconstruct minimal path in tree");

    size_t n = 8;
    size_t *dist = malloc(n * sizeof(size_t));
    size_t *parent = malloc(n * sizeof(size_t));
    size_t *path_out = malloc(n * sizeof(size_t));
    REQUIRE(dist && parent && path_out, "alloc arrays for unique-path test");

    Graph *g = create_graph(n, false);
    REQUIRE(g != NULL, "create_graph for unique-path test");
    for (size_t i = 0; i + 1 < n; ++i) add_edge(g, i, i + 1);

    size_t goal = n - 1;
    size_t start = 0;

    REQUIRE(bfs_precompute_to_goal(g, goal, dist, parent), "precompute should succeed on tree");

    ssize_t len = bfs_reconstruct_path(g, start, goal, parent, path_out);
    REQUIRE(len >= 1, "reconstruct returned non-positive length");

    REQUIRE(path_out[0] == start, "path_out[0] == start");
    REQUIRE(path_out[len - 1] == goal, "path_out[last] == goal");

    REQUIRE((size_t)(len - 1) == dist[start], "len-1 must equal dist[start]");

    free(dist);
    free(parent);
    free(path_out);
    free_graph(g);

    print_test_ok();
}

static void test_bfs_grid_reference(void) {
    print_test_line_prefix("1.4", "grid reference (w x h): distance and minimal path");

    size_t w = 6, h = 5; 
    Graph *g = build_grid_graph(w, h);
    REQUIRE(g != NULL, "build grid graph for reference test");

    size_t start = 0;
    size_t goal = w * h - 1;

    size_t *dist = malloc(g->order * sizeof(size_t));
    size_t *parent = malloc(g->order * sizeof(size_t));
    size_t *path_out = malloc(g->order * sizeof(size_t));
    REQUIRE(dist && parent && path_out, "alloc arrays for grid reference test");

    REQUIRE(bfs_precompute_to_goal(g, goal, dist, parent), "precompute should succeed on grid");

    size_t expected_distance = (w - 1) + (h - 1);
    REQUIRE(dist[start] == expected_distance, "dist[start] equals Manhattan distance for grid");

    ssize_t len = bfs_reconstruct_path(g, start, goal, parent, path_out);
    REQUIRE(len == (ssize_t)(expected_distance + 1), "reconstructed path length equals distance+1");
    REQUIRE(path_out[0] == start, "path starts at start");
    REQUIRE(path_out[len - 1] == goal, "path ends at goal");

    free(dist);
    free(parent);
    free(path_out);
    free_graph(g);

    print_test_ok();
}

static void test_bfs_precompute_consistency(void) {
    print_test_line_prefix("1.5", "precompute consistency: parent & dist invariants");

    size_t w = 8, h = 6;
    Graph *g = build_grid_graph(w, h);
    REQUIRE(g != NULL, "build grid for precompute consistency");

    size_t goal = g->order - 1;

    size_t *dist = malloc(g->order * sizeof(size_t));
    size_t *parent = malloc(g->order * sizeof(size_t));
    REQUIRE(dist && parent, "alloc arrays for precompute consistency");

    REQUIRE(bfs_precompute_to_goal(g, goal, dist, parent), "precompute should succeed on grid");

    REQUIRE(parent[goal] == goal, "parent[goal] == goal sentinel");

    /*
     *  - parent[v] is a valid index (< order)
     *  - following parent pointers reaches goal within at most order steps
     *  - if v != goal then dist[v] == dist[parent[v]] + 1
     */

    for (size_t v = 0; v < g->order; ++v) {
        if (dist[v] == SIZE_MAX) continue; 

        REQUIRE(parent[v] < g->order, "parent[v] in range for reached v");
        size_t cur = v;
        bool reached_goal = false;
        for (size_t step = 0; step < g->order; ++step) {
            if (cur >= g->order) break; 
            if (cur == parent[cur]) { 
                if (cur == goal) reached_goal = true;
                break;
            }
            cur = parent[cur];
        }
        REQUIRE(reached_goal, "following parent pointers reaches goal");

        if (v != goal) {
            REQUIRE(dist[v] == dist[parent[v]] + 1, "dist[v] == dist[parent[v]] + 1 for reached v != goal");
        }
    }

    free(dist);
    free(parent);
    free_graph(g);

    print_test_ok();
}

static void test_bfs_precompute_oob_and_reconstruct(void) {
    print_test_line_prefix("1.6", "bfs_precompute out-of-bounds goal & reconstruct edge cases");

    size_t n = 4;
    size_t *dist = malloc(n * sizeof(size_t));
    size_t *parent = malloc(n * sizeof(size_t));
    size_t *path_out = malloc(n * sizeof(size_t));
    REQUIRE(dist && parent && path_out, "alloc arrays for oob test");

    Graph *g = create_graph(n, false);
    REQUIRE(g != NULL, "create_graph for oob test");

    REQUIRE(!bfs_precompute_to_goal(g, g->order, dist, parent), "goal >= order should return false");

    /* If precompute failed, parent[] may be uninitialized. Set it to SIZE_MAX
     * to represent 'not computed' so reconstruction deterministically fails. */
    for (size_t i = 0; i < n; ++i) parent[i] = SIZE_MAX;
    ssize_t len_bad = bfs_reconstruct_path(g, 0, 0, parent, path_out);
    REQUIRE(len_bad == -1, "reconstruct should return -1 when precompute not done (parent all SIZE_MAX)");

    REQUIRE(bfs_precompute_to_goal(g, 0, dist, parent), "precompute with valid goal should succeed");
    ssize_t len_oob_start = bfs_reconstruct_path(g, (size_t)g->order, 0, parent, path_out);
    REQUIRE(len_oob_start == -1, "reconstruct should return -1 when start >= g->order");

    free(dist);
    free(parent);
    free(path_out);
    free_graph(g);

    print_test_ok();
}

static void test_bfs_reconstruct_buffer_safety(void) {
    print_test_line_prefix("1.7", "reconstruct buffer safety: writes only L entries");

    size_t n = 10;
    Graph *g = create_graph(n, false);
    REQUIRE(g != NULL, "create_graph for buffer safety test");
    for (size_t i = 0; i + 1 < n; ++i) add_edge(g, i, i + 1);

    size_t *dist = malloc(n * sizeof(size_t));
    size_t *parent = malloc(n * sizeof(size_t));
    size_t *path_out = malloc(n * sizeof(size_t));
    REQUIRE(dist && parent && path_out, "alloc arrays for buffer safety");

    for (size_t i = 0; i < n; ++i) path_out[i] = SIZE_MAX;

    size_t start = 0, goal = n - 1;
    REQUIRE(bfs_precompute_to_goal(g, goal, dist, parent), "precompute succeeded on chain");

    ssize_t L = bfs_reconstruct_path(g, start, goal, parent, path_out);
    REQUIRE(L > 0 && (size_t)L <= n, "reconstruct returned valid length");

    for (size_t i = 0; i < (size_t)L; ++i) REQUIRE(path_out[i] != SIZE_MAX, "path_out[i] written for i < L");
    for (size_t i = (size_t)L; i < n; ++i) REQUIRE(path_out[i] == SIZE_MAX, "path_out[i] untouched for i >= L");

    free(dist);
    free(parent);
    free(path_out);
    free_graph(g);

    print_test_ok();
}

static void test_bfs_perf_and_memory(void) {
    print_test_line_prefix("1.8", "perf & memory: large grid and maze (100x100)");

    size_t w = 100, h = 100;

    Graph *ggrid = build_grid_graph(w, h);
    REQUIRE(ggrid != NULL, "build large grid");
    size_t *dist = malloc(ggrid->order * sizeof(size_t));
    size_t *parent = malloc(ggrid->order * sizeof(size_t));
    REQUIRE(dist && parent, "alloc arrays for large grid");

    double t0 = now_monotonic_test();
    REQUIRE(bfs_precompute_to_goal(ggrid, ggrid->order - 1, dist, parent), "precompute on large grid succeeded");
    double t1 = now_monotonic_test();
    double grid_dt = t1 - t0;

    print_test_ok();

    printf("\nPerformance results:\n");
    printf("  %-8s | %-10s | %-12s\n", "Type", "Size", "Precompute time");
    printf("  %-8s-+-%-10s-+-%-12s\n", "--------", "----------", "------------");
    printf("  %-8s | %4zux%4zu | %10.6f s\n", "grid", w, h, grid_dt);

    free(dist);
    free(parent);
    free_graph(ggrid);

    Graph *gmaze = generate_maze(w, h);
    REQUIRE(gmaze != NULL, "generate_maze(100,100)");
    dist = malloc(gmaze->order * sizeof(size_t));
    parent = malloc(gmaze->order * sizeof(size_t));
    REQUIRE(dist && parent, "alloc arrays for maze precompute");

    t0 = now_monotonic_test();
    REQUIRE(bfs_precompute_to_goal(gmaze, gmaze->order - 1, dist, parent), "precompute on maze succeeded");
    t1 = now_monotonic_test();
    double maze_dt = t1 - t0;
    printf("  %-8s | %4zux%4zu | %10.6f s\n", "maze", w, h, maze_dt);

    printf("\n  Summary: grid %.6f s, maze %.6f s (precompute %s O(V+E) expected)\n",
        grid_dt, maze_dt, "linear");

    free(dist);
    free(parent);
    free_graph(gmaze);

    printf("\n  Note: run under Valgrind or AddressSanitizer to check for memory leaks and errors\n");
}

static int run_all_tests(void) {
    printf("=== BFS tests ===\n");
    test_bfs_precompute_invalid_arguments();
    test_bfs_inaccessible_two_components();
    test_bfs_unique_path_tree();
    test_bfs_grid_reference();
    test_bfs_precompute_consistency();
    test_bfs_precompute_oob_and_reconstruct();
    test_bfs_reconstruct_buffer_safety();
    test_bfs_perf_and_memory();

    printf("All bfs tests passed: ");
    fflush(stdout);
    print_test_ok();
    return 0;
}

int main(void) {
    return run_all_tests();
}
 
