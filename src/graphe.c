#include "../inc/graphe.h"

/* Create and initialize a graph structure. Returns NULL on failure. */
Graph* create_graph(size_t order, bool oriented) {
    if (order == 0) return NULL;
    
    Graph *g = malloc(sizeof(Graph));
    if (!g) return NULL;

    g->oriented = oriented;
    g->order = order;
    g->nb_edges = 0;

    g->list = malloc(order * sizeof(AdjList));
    if (!g->list) {
        free(g);
        return NULL;
    }

    for (size_t i = 0; i < order; i++) {
        g->list[i].nb_neighbors = 0;
        g->list[i].capacity = 0;
        g->list[i].neighbors = NULL;
    }

    return g;
}

void free_graph(Graph *g) {
    if (!g) return;
    for (size_t i = 0; i < g->order; i++) {
        free(g->list[i].neighbors);
    }
    free(g->list);
    free(g);
}

/* Check whether edge u->v exists. Performs bounds checking. */
bool edge_exists(const Graph *g, size_t u, size_t v) {
    if (!g || u >= g->order || v >= g->order) return false;
    for (size_t i = 0; i < g->list[u].nb_neighbors; i++) {
        if (g->list[u].neighbors[i] == v)
            return true;
    }
    return false;
}

/* Add an edge (u->v). For undirected graphs, add symmetric entry too. */
void add_edge(Graph *g, size_t u, size_t v) {
    if (!g || u >= g->order || v >= g->order || u == v) return;
    if (edge_exists(g, u, v)) return;

    AdjList *adj_u = &g->list[u];
    if (adj_u->nb_neighbors >= adj_u->capacity) {
        size_t new_cap = (adj_u->capacity == 0) ? 2 : adj_u->capacity * 2;
        size_t *tmp = realloc(adj_u->neighbors, new_cap * sizeof(size_t));
        if (!tmp) return;
        adj_u->neighbors = tmp;
        adj_u->capacity = new_cap;
    }
    adj_u->neighbors[adj_u->nb_neighbors++] = v;

    if (!g->oriented) {
        AdjList *adj_v = &g->list[v];
        if (adj_v->nb_neighbors >= adj_v->capacity) {
            size_t new_cap = (adj_v->capacity == 0) ? 2 : adj_v->capacity * 2;
            size_t *tmp = realloc(adj_v->neighbors, new_cap * sizeof(size_t));
            if (!tmp) return;
            adj_v->neighbors = tmp;
            adj_v->capacity = new_cap;
        }
        adj_v->neighbors[adj_v->nb_neighbors++] = u;
    }

    g->nb_edges++;
}

/* Remove edge u->v; uses swap-with-last for O(1) removal from array. */
void remove_edge(Graph *g, size_t u, size_t v) {
    if (!g || u >= g->order || v >= g->order) return;
    bool removed = false;

    AdjList *adj_u = &g->list[u];
    for (size_t i = 0; i < adj_u->nb_neighbors; i++) {
        if (adj_u->neighbors[i] == v) {
            adj_u->neighbors[i] = adj_u->neighbors[--adj_u->nb_neighbors];
            removed = true;
            break;
        }
    }

    /* For undirected graphs, remove the symmetric entry as well. */
    if (!g->oriented) {
        AdjList *adj_v = &g->list[v];
        for (size_t i = 0; i < adj_v->nb_neighbors; i++) {
            if (adj_v->neighbors[i] == u) {
                adj_v->neighbors[i] = adj_v->neighbors[--adj_v->nb_neighbors];
                /* don't change 'removed' here; it's already true if u->v existed */
                break;
            }
        }
    }

    if (removed && g->nb_edges > 0) {
        g->nb_edges--;
    }
}

void print_graph(const Graph *g) {
    if (!g) return;
    printf("Graphe (%zu sommets, %zu arêtes):\n", g->order, g->nb_edges);
    for (size_t i = 0; i < g->order; i++) {
        printf("%zu :", i);
        for (size_t j = 0; j < g->list[i].nb_neighbors; j++) {
            printf(" %zu", g->list[i].neighbors[j]);
        }
        printf("\n");
    }
}

