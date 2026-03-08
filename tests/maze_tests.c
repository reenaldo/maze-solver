#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "../inc/graphe.h"
#include "../inc/bfs.h"
#include "../inc/maze.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

static const char *g_test_id = NULL;
static const char *g_test_desc = NULL;

#define TEST_LINE_WIDTH 100

#define COLOR_GREEN "\x1b[32m"
#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
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


static size_t expected_grid_edges(size_t w, size_t h) {
    return h * (w - 1) + w * (h - 1);
}

static bool is_connected_via_bfs(const Graph *g) {
    if (!g || g->order == 0) return false;
    size_t n = g->order;
    size_t *dist = malloc(n * sizeof(size_t));
    size_t *parent = malloc(n * sizeof(size_t));
    if (!dist || !parent) { free(dist); free(parent); return false; }
    if (!bfs_precompute_to_goal(g, 0, dist, parent)) { free(dist); free(parent); return false; }
    bool all_reached = true;
    for (size_t i = 0; i < n; i++) {
        if (dist[i] == SIZE_MAX) { all_reached = false; break; }
    }
    free(dist); free(parent);
    return all_reached;
}

static Graph* build_grid_graph(size_t w, size_t h) {
    Graph *g = create_graph(w * h, false);
    if (!g) return NULL;
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t id = y * w + x;
            if (x + 1 < w) add_edge(g, id, id + 1);
            if (y + 1 < h) add_edge(g, id, id + w);
        }
    }
    return g;
}

static void test_create_graph_basic(void) {
    print_test_line_prefix("1.1", "create_graph basic");
    Graph *g = create_graph(10, false);
    REQUIRE(g != NULL, "create_graph returned NULL");
    REQUIRE(g->order == 10, "order set");
    REQUIRE(g->nb_edges == 0, "nb_edges is zero");
    for (size_t i = 0; i < g->order; i++) {
        REQUIRE(g->list[i].nb_neighbors == 0, "adj list empty at start");
        REQUIRE(g->list[i].capacity == 0, "adj list capacity zero at start");
        REQUIRE(g->list[i].neighbors == NULL, "adj list pointer NULL at start");
    }
    free_graph(g);
    print_test_ok();
}

static void test_create_grid_graph_and_connectivity(void) {
    print_test_line_prefix("1.2", "create_grid_graph and connectivity");
    size_t w = 5, h = 4;
    Graph *g = build_grid_graph(w, h);
    REQUIRE(g != NULL, "grid creation");
    REQUIRE(g->order == w * h, "grid order");
    size_t expected = expected_grid_edges(w, h);
    REQUIRE(g->nb_edges == expected, "grid edges count");
    REQUIRE(is_connected_via_bfs(g), "grid should be connected");
    free_graph(g);
    print_test_ok();
}

static void test_add_remove_edge(void) {
    print_test_line_prefix("1.3", "add_edge & remove_edge consistency");
    Graph *g = create_graph(6, false);
    REQUIRE(g != NULL, "create_graph for add/remove test");

    REQUIRE(g->nb_edges == 0, "nb_edges == 0 initially");

    add_edge(g, 0, 1);
    REQUIRE(g->nb_edges == 1, "nb_edges == 1 after add_edge(0,1)");
    REQUIRE(edge_exists(g, 0, 1), "edge 0-1 exists");
    REQUIRE(edge_exists(g, 1, 0), "edge 1-0 exists (undirected)");
    REQUIRE(g->list[0].nb_neighbors >= 1, "vertex 0 has neighbor(s)");
    REQUIRE(g->list[1].nb_neighbors >= 1, "vertex 1 has neighbor(s)");

    add_edge(g, 0, 1);
    REQUIRE(g->nb_edges == 1, "duplicate add_edge doesn't increase nb_edges");

    add_edge(g, 2, 3);
    REQUIRE(g->nb_edges == 2, "nb_edges == 2 after another add");
    REQUIRE(edge_exists(g, 2, 3) && edge_exists(g, 3, 2), "edge 2-3 exists both ways");

    remove_edge(g, 0, 1);
    REQUIRE(!edge_exists(g, 0, 1), "edge 0-1 removed");
    REQUIRE(!edge_exists(g, 1, 0), "edge 1-0 removed (undirected)");
    REQUIRE(g->nb_edges == 1, "nb_edges decremented after remove");

    remove_edge(g, 0, 5); // no-op
    REQUIRE(g->nb_edges == 1, "remove non-existing edge doesn't change nb_edges");

    free_graph(g);

    Graph *dg = create_graph(4, true);
    REQUIRE(dg != NULL, "create directed graph");
    add_edge(dg, 0, 1);
    REQUIRE(edge_exists(dg, 0, 1), "directed edge 0->1 exists");
    REQUIRE(!edge_exists(dg, 1, 0), "directed edge 1->0 does not exist");
    REQUIRE(dg->nb_edges == 1, "directed nb_edges == 1");
    remove_edge(dg, 0, 1);
    REQUIRE(!edge_exists(dg, 0, 1), "directed edge removed");
    REQUIRE(dg->nb_edges == 0, "directed nb_edges == 0 after remove");
    free_graph(dg);

    print_test_ok();
}

static void test_divide_and_connectivity(void) {
    print_test_line_prefix("1.4", "divide() walls & connectivity");
    srand(12345);

    size_t w = 11, h = 9;
    Graph *g = build_grid_graph(w, h);
    REQUIRE(g != NULL, "build grid for divide test");

    size_t full = expected_grid_edges(w, h);
    divide(g, 0, 0, w - 1, h - 1, w, h);

    REQUIRE(g->nb_edges < full, "divide should remove some edges (create walls)");
    REQUIRE(is_connected_via_bfs(g), "maze must remain connected after division");

    free_graph(g);
    print_test_ok();
}

static void test_is_connected_bfs(void) {
    print_test_line_prefix("1.5", "is_connected (BFS full traversal)");

    Graph *g1 = build_grid_graph(4, 3);
    REQUIRE(g1 != NULL, "build grid graph for is_connected");
    REQUIRE(is_connected_via_bfs(g1), "grid graph should be connected");
    free_graph(g1);

    Graph *g2 = create_graph(4, false);
    REQUIRE(g2 != NULL, "create_graph for disconnected test");
    add_edge(g2, 0, 1);
    add_edge(g2, 2, 3);
    REQUIRE(!is_connected_via_bfs(g2), "graph with two components should not be connected");
    free_graph(g2);

    Graph *g3 = create_graph(1, false);
    REQUIRE(g3 != NULL, "create_graph single node");
    REQUIRE(is_connected_via_bfs(g3), "single-node graph should be connected");
    free_graph(g3);

    print_test_ok();
}

static size_t graph_fingerprint(const Graph *g) {
    if (!g) return 0;
    const unsigned long FNV_offset = 1469598103934665603UL;
    const unsigned long FNV_prime  = 1099511628211UL;
    unsigned long h = FNV_offset;
    for (size_t v = 0; v < g->order; v++) {
        /* mix vertex id */
        h = (h ^ (v + 1)) * FNV_prime;
        AdjList al = g->list[v];
        for (size_t k = 0; k < al.nb_neighbors; k++) {
            size_t nb = al.neighbors[k];
            h = (h ^ (nb + 1)) * FNV_prime;
        }
    }
    return (size_t)h;
}

static void test_random_seed_variability(void) {
    print_test_line_prefix("1.6", "randomness & reproducibility (seeds)");
    size_t w = 15, h = 11; 

    size_t seeds[] = {42, 43, 44, 12345, 99999};
    size_t nseeds = sizeof(seeds) / sizeof(seeds[0]);
    size_t *fps = malloc(nseeds * sizeof(size_t));
    if (!fps) { fprintf(stderr, "Out of memory in randomness test\n"); exit(EXIT_FAILURE); }

    for (size_t i = 0; i < nseeds; ++i) {
        unsigned int s = (unsigned int)seeds[i];
        srand(s);
        Graph *g1 = generate_maze(w, h);
        REQUIRE(g1 != NULL, "generate_maze g1");
        REQUIRE(is_connected_via_bfs(g1), "g1 connected");
        fps[i] = graph_fingerprint(g1);
        free_graph(g1);

        srand(s);
        Graph *g2 = generate_maze(w, h);
        REQUIRE(g2 != NULL, "generate_maze g2");
        REQUIRE(is_connected_via_bfs(g2), "g2 connected");
        size_t f2 = graph_fingerprint(g2);
        free_graph(g2);

        REQUIRE(fps[i] == f2, "same seed reproduces same maze fingerprint");
    }

    size_t unique = 0;
    for (size_t i = 0; i < nseeds; ++i) {
        bool found = false;
        for (size_t j = 0; j < i; ++j) {
            if (fps[i] == fps[j]) { found = true; break; }
        }
        if (!found) unique++;
    }

    if (unique < nseeds) {
        printf(COLOR_YELLOW "(warning) %zu/%zu unique fingerprints for different seeds (possible collision)" COLOR_RESET "\n", unique, nseeds);
    }

    free(fps);
    print_test_ok();
}

static double now_monotonic_test(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void test_performance_generate_large_mazes(void) {
    print_test_line_prefix("1.7", "performance: generate large mazes and measure runtime");

    size_t sizes[] = {30, 50, 80, 100};
    size_t nsizes = sizeof(sizes) / sizeof(sizes[0]);
    double *times = malloc(nsizes * sizeof(double));
    if (!times) { print_test_fail("out of memory for times", __LINE__); }

    for (size_t i = 0; i < nsizes; i++) {
        size_t s = sizes[i];
        size_t w = s, h = s;
        srand((unsigned int)(1234 + (unsigned)i));
        double t0 = now_monotonic_test();
        Graph *g = generate_maze(w, h);
        double t1 = now_monotonic_test();
        REQUIRE(g != NULL, "generate_maze returned non-NULL");
        double dt = t1 - t0;
        REQUIRE(is_connected_via_bfs(g), "generated maze should be connected");
        free_graph(g);
        times[i] = dt;
    }

    print_test_ok();

    printf("\nPerformance results (generation time):\n");
    printf("  %-8s | %-9s | %-12s\n", "Index", "Size", "Time");
    printf("  %-8s-+-%-9s-+-%-12s\n", "--------", "---------", "------------");
    for (size_t i = 0; i < nsizes; ++i) {
        size_t s = sizes[i];
        printf("  %4zu     | %4zux%4zu  | %10.6f s\n", i + 1, s, s, times[i]);
    }
    printf("\n");
    free(times);
}

static int run_all_tests(void) {
    printf("=== Maze graph tests ===\n");
    test_create_graph_basic();
    test_create_grid_graph_and_connectivity();
    test_add_remove_edge();
    test_divide_and_connectivity();
    test_is_connected_bfs();
    test_random_seed_variability();
    test_performance_generate_large_mazes();
    printf("All maze tests passed: ");
    fflush(stdout);
    print_test_ok();
    return 0;
}

int main(void) {
    return run_all_tests();
}
