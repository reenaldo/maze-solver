#define _POSIX_C_SOURCE 200809L
#define ANS_RESET "\x1b[0m"
#define ANS_BOLD  "\x1b[1m"
#define ANS_CYAN  "\x1b[36m"
#define ANS_GREEN "\x1b[32m"
#define ANS_YELLOW "\x1b[33m"

#include "../inc/maze.h"
#include "../inc/bfs.h"
#include "../inc/json_export.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/wait.h>

double now_monotonic(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void format_time(char *buf, size_t bufsz, double t) {
    if (t >= 1.0) {
        snprintf(buf, bufsz, "%.6f s", t);
    } else if (t >= 1e-3) {
        snprintf(buf, bufsz, "%.3f ms", t * 1e3);
    } else if (t >= 1e-6) {
        snprintf(buf, bufsz, "%.3f \u00B5s", t * 1e6);
    } else {
        snprintf(buf, bufsz, "%.3f ns", t * 1e9);
    }
}

static void print_kv_colored(int is_tty, const char *label, const char *value) {
    if (is_tty) {
        printf("  %-28s %s%s%s\n", label, ANS_BOLD ANS_GREEN, value, ANS_RESET);
    } else {
        printf("  %-28s %s\n", label, value);
    }
}

static void print_time_human_colored(int is_tty, const char *label, double t) {
    char human[64];
    char seconds[64];
    format_time(human, sizeof(human), t);
    snprintf(seconds, sizeof(seconds), "%.9f s", t);

    /* Try to allocate exact-size buffer first (no warning possible). */
    int needed = snprintf(NULL, 0, "%s (%s)", human, seconds) + 1; /* includes NUL */
    char *combined = malloc((size_t)needed);
    if (combined) {
        /* dynamic buffer — safe to use snprintf */
        snprintf(combined, (size_t)needed, "%s (%s)", human, seconds);
        print_kv_colored(is_tty, label, combined);
        free(combined);
        return;
    }

    /* Fallback: build the string manually into a fixed buffer without using
     * snprintf (avoids -Wformat-truncation). We produce "%s (%s)" truncated
     * safely if necessary.
     */
    char combined_buf[128];
    size_t rem = sizeof(combined_buf);
    size_t pos = 0;

    size_t hlen = strlen(human);
    size_t copy_h = (hlen < rem - 1) ? hlen : (rem - 1);
    if (copy_h > 0) {
        memcpy(combined_buf + pos, human, copy_h);
        pos += copy_h;
    }

    if (pos + 2 < rem) {
        combined_buf[pos++] = ' ';
        combined_buf[pos++] = '(';
    } else if (pos < rem) {
        combined_buf[pos] = '\0';
        print_kv_colored(is_tty, label, combined_buf);
        return;
    } else {
        combined_buf[rem - 1] = '\0';
        print_kv_colored(is_tty, label, combined_buf);
        return;
    }

    size_t slen = strlen(seconds);
    size_t avail = rem - pos - 2; 
    size_t copy_s = (slen < avail) ? slen : avail;
    if (copy_s > 0) {
        memcpy(combined_buf + pos, seconds, copy_s);
        pos += copy_s;
    }

    if (pos < rem - 1) {
        combined_buf[pos++] = ')';
    } else if (pos == rem - 1) {
        combined_buf[pos - 1] = ')';
    }
    combined_buf[(pos < rem) ? pos : rem - 1] = '\0';
    print_kv_colored(is_tty, label, combined_buf);
}

static void print_prompt(int is_tty, const char *msg) {
    if (is_tty) {
        printf("%s%s>>> %s%s", ANS_BOLD ANS_CYAN, "", msg, ANS_RESET);
    } else {
        printf(">>> %s", msg);
    }
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    
    srand((unsigned int)(time(NULL) ^ clock()));
    int is_tty = isatty(STDOUT_FILENO);
    size_t width = 0, height = 0;
    if (argc == 3) {
        width = (size_t)atoi(argv[1]);
        height = (size_t)atoi(argv[2]);
        if (width == 0 || height == 0) {
            fprintf(stderr, "Error: width and height must be > 0.\n");
            return EXIT_FAILURE;
        }
    } else {
        while (1) {
            print_prompt(is_tty, "Enter maze dimensions (width height): ");
            if (scanf("%zu %zu", &width, &height) != 2) {
                int c;
                if (is_tty) fprintf(stderr, "%s%sInvalid input — please enter two positive integers.%s\n", ANS_BOLD ANS_YELLOW, "", ANS_RESET);
                else fprintf(stderr, "Invalid input — please enter two positive integers.\n");
                while ((c = getchar()) != '\n' && c != EOF) {}
                continue;
            }
            if (width == 0 || height == 0) {
                fprintf(stderr, "Error: width and height must be > 0.\n");
                continue;
            }
            break;
        }
    }

    size_t goal = cell_id(width - 1, height - 1, width);

    /* Ask for a start point : re-prompt until valid coordinates inside the maze are given */
    size_t sx = 0, sy = 0;
    size_t start = 0;
    while (1) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Enter start coordinates (x y) [0..%zu] [0..%zu]: ", width - 1, height - 1);
        print_prompt(is_tty, buf);
        if (scanf("%zu %zu", &sx, &sy) != 2) {
            int c;
            if (is_tty) fprintf(stderr, "%s%sInvalid input — please enter two integers for x and y.%s\n", ANS_BOLD ANS_YELLOW, "", ANS_RESET);
            else fprintf(stderr, "Invalid input — please enter two integers for x and y.\n");
            while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }
        if (sx >= width || sy >= height) {
            fprintf(stderr, "Coordinates (%zu,%zu) are outside the maze (max %zux%zu). Try again.\n", 
                sx, sy, width, height);
                continue;
        }

        start = cell_id(sx, sy, width);
        if (start == goal) {
            if (sy + 1 < height) {
                sy++;
                start = cell_id(sx, sy, width);
                printf("Start was goal; shifted to (%zu,%zu) -> id=%zu\n", sx, sy, start);
            } else if (sx + 1 < width) {
                sx++;
                start = cell_id(sx, sy, width);
                printf("Start was goal; shifted to (%zu,%zu) -> id=%zu\n", sx, sy, start);
            } else {
                fprintf(stderr, "Start equals goal and cannot be shifted in this tiny maze. Please enter a different start.\n");
                continue;
            }
        }
        break;
    }

    double start_gen = now_monotonic();
    Graph *maze = generate_maze(width, height);
    double end_gen = now_monotonic();
    clock_t start_graph = clock();
    clock_t end_graph = clock();
    double graph_time = (double)(end_graph - start_graph) / CLOCKS_PER_SEC;

    double t_gen = end_gen - start_gen;
    double n = (double)(width * height);


    /* Maze Summary */
    if (is_tty) printf("%s%s=== Maze summary ===%s\n", ANS_BOLD ANS_CYAN, "", ANS_RESET);
    else printf("=== Maze summary ===\n");

    char dims_buf[64];
    snprintf(dims_buf, sizeof(dims_buf), "%zux%zu", width, height);
    print_kv_colored(is_tty, "Dimensions (width x height)", dims_buf);

    char cells_buf[64];
    snprintf(cells_buf, sizeof(cells_buf), "%zu", (size_t)n);
    print_kv_colored(is_tty, "Number of cells", cells_buf);

    char start_buf[64];
    snprintf(start_buf, sizeof(start_buf), "%zu,%zu (id=%zu)", sx, sy, start);
    print_kv_colored(is_tty, "Start (x,y and id)", start_buf);

    size_t gx = (width == 0) ? 0 : (goal % width);
    size_t gy = (width == 0) ? 0 : (goal / width);
    char goal_buf[64];
    snprintf(goal_buf, sizeof(goal_buf), "%zu,%zu (id=%zu)", gx, gy, goal);
    print_kv_colored(is_tty, "Goal (x,y and id)", goal_buf);

    print_time_human_colored(is_tty, "Generation time", t_gen);
    print_time_human_colored(is_tty, "Display time (graph)", graph_time);
    print_time_human_colored(is_tty, "Total time", t_gen + graph_time);
    print_time_human_colored(is_tty, "Average time per cell", t_gen / n);
    
    size_t *dist   = malloc(n * sizeof(size_t));
    size_t *parent = malloc(n * sizeof(size_t));
    if (!dist || !parent) {
        fprintf(stderr, "Allocation of dist/parent failed\n");
        free(dist); free(parent);
        free_graph(maze);
        return 1;
    }

    double start_bfs = now_monotonic();

    if (!bfs_precompute_to_goal(maze, goal, dist, parent)) {
        fprintf(stderr, "bfs_precompute_to_goal failed\n");
        free(dist); free(parent);
        free_graph(maze);
        return 1;
    }

    double end_bfs = now_monotonic();
    double t_bfs = end_bfs - start_bfs;

    size_t *path = malloc(n * sizeof(size_t));
    if (!path) {
        fprintf(stderr, "Allocation of path failed\n");
        free(dist); free(parent);
        free_graph(maze);
        return 1;
    }

    double start_path = now_monotonic();
    (void)bfs_reconstruct_path(maze, start, goal, parent, path);
    double end_path = now_monotonic();
    double t_path = end_path - start_path;

    ssize_t path_len = bfs_reconstruct_path(maze, start, goal, parent, path);
    char path_buf[64];
    if (path_len < 0) snprintf(path_buf, sizeof(path_buf), "(no path)");
    else snprintf(path_buf, sizeof(path_buf), "%zd vertices", path_len);
    print_kv_colored(is_tty, "Path length (vertices)", path_buf);

    print_time_human_colored(is_tty, "Total BFS time", t_bfs + t_path);

    /* Export JSON describing the maze and BFS steps for external visualization */
    int jres = export_maze_bfs_json(maze, width, height, start, goal, "maze_bfs_steps.json");
    if (jres != 0) {
        fprintf(stderr, "Warning: failed to export JSON (code %d)\n", jres);
    } else {
        int rc = system("python3 scripts/visualize_maze.py maze_bfs_steps.json");
        if (rc == -1) {
            perror("system(python3 visualize)");
        } else if (rc != 0) {
            /* Try fallback to `python` if `python3` failed / not available. Use
             * the return value so we don't ignore the result (avoids
             * -Wunused-result) and report errors when possible. */
            int rc2 = system("python scripts/visualize_maze.py maze_bfs_steps.json");
            if (rc2 == -1) {
                perror("system(python fallback)");
            } else {
                #ifdef WIFEXITED
                if (!WIFEXITED(rc2) || WEXITSTATUS(rc2) != 0) {
                    fprintf(stderr, "visualize script returned non-zero exit/status: %d\n", rc2);
                }
                #else
                (void)rc2;
                #endif
            }
        }
    }

    free(path);
    free(dist);
    free(parent);
    free_graph(maze);
    return 0;
}

