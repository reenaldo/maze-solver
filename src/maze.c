#include "../inc/maze.h"

size_t cell_id(size_t x, size_t y, size_t width) {
    return y * width + x;
}

static void connect_all(Graph *g, size_t width, size_t height) {
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t id = cell_id(x, y, width);
            if (x + 1 < width) add_edge(g, id, cell_id(x + 1, y, width));
            if (y + 1 < height) add_edge(g, id, cell_id(x, y + 1, width));
        }
    }
}

// Recursive division algorithm: carve walls by removing edges, leaving one hole per wall
void divide(Graph *g, size_t x0, size_t y0, size_t x1, size_t y1, size_t width, size_t height) {
    if (x1 <= x0 || y1 <= y0) return;

    bool horizontal = (x1 - x0 < y1 - y0);

    if (horizontal) {
        size_t y = y0 + rand() % (y1 - y0);
        size_t hole = x0 + rand() % (x1 - x0 + 1);
        for (size_t x = x0; x <= x1; x++) {
            if (x == hole) continue;
            remove_edge(g, cell_id(x, y, width), cell_id(x, y + 1, width));
        }
        divide(g, x0, y0, x1, y, width, height);
        divide(g, x0, y + 1, x1, y1, width, height);
    } else {
        size_t x = x0 + rand() % (x1 - x0);
        size_t hole = y0 + rand() % (y1 - y0 + 1);
        for (size_t y = y0; y <= y1; y++) {
            if (y == hole) continue;
            remove_edge(g, cell_id(x, y, width), cell_id(x + 1, y, width));
        }
        divide(g, x0, y0, x, y1, width, height);
        divide(g, x + 1, y0, x1, y1, width, height);
    }
}

// Create a new graph (grid) and generate a maze using recursive division
Graph* generate_maze(size_t width, size_t height) {
    Graph *g = create_graph(width * height, false);
    connect_all(g, width, height);
    divide(g, 0, 0, width - 1, height - 1, width, height);
    return g;
}

