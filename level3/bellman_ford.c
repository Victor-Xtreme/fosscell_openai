#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define INF LLONG_MAX

typedef struct {
    int src, dest, weight;
} Edge;

/* CSR (compressed sparse row) graph: edges of vertex u are stored at
 * indices offset[u] .. offset[u+1]-1 of dest[] / weight[]. */
typedef struct {
    int V, E;
    int *offset;   /* V + 1 entries */
    int *dest;     /* E entries */
    int *weight;   /* E entries */
} Graph;

typedef enum {
    SP_OK = 0,
    SP_NEGATIVE_CYCLE = 1,
    SP_ERROR = -1
} SpStatus;

static void graph_destroy(Graph *g)
{
    if (g == NULL) {
        return;
    }
    free(g->offset);
    free(g->dest);
    free(g->weight);
    free(g);
}

static Graph *graph_create(int V, const Edge *edges, int E)
{
    if (V <= 0 || E < 0 || edges == NULL) {
        return NULL;
    }

    Graph *g = calloc(1, sizeof *g);
    if (g == NULL) {
        return NULL;
    }
    g->V = V;
    g->E = E;

    size_t edge_slots = (E > 0) ? (size_t)E : 1;
    g->offset = calloc((size_t)V + 1, sizeof *g->offset);
    g->dest   = calloc(edge_slots, sizeof *g->dest);
    g->weight = calloc(edge_slots, sizeof *g->weight);
    int *next = malloc((size_t)V * sizeof *next);
    if (!g->offset || !g->dest || !g->weight || !next) {
        free(next);
        graph_destroy(g);
        return NULL;
    }

    /* Count out-degrees, validating endpoints. */
    for (int i = 0; i < E; i++) {
        if (edges[i].src < 0 || edges[i].src >= V ||
            edges[i].dest < 0 || edges[i].dest >= V) {
            free(next);
            graph_destroy(g);
            return NULL;
        }
        g->offset[edges[i].src + 1]++;
    }

    /* Prefix sums give each vertex's starting slot. */
    for (int u = 0; u < V; u++) {
        g->offset[u + 1] += g->offset[u];
        next[u] = g->offset[u];
    }

    /* Place edges into their slots. */
    for (int i = 0; i < E; i++) {
        int slot = next[edges[i].src]++;
        g->dest[slot]   = edges[i].dest;
        g->weight[slot] = edges[i].weight;
    }

    free(next);
    return g;
}

/* Fills dist[0..V-1]; unreachable vertices get INF. */
static SpStatus shortest_paths(const Graph *g, int src, long long *dist)
{
    if (src < 0 || src >= g->V) {
        return SP_ERROR;
    }

    int V = g->V;
    int *queue    = malloc((size_t)V * sizeof *queue);   /* circular buffer */
    int *path_len = calloc((size_t)V, sizeof *path_len); /* edges on best path */
    char *in_queue = calloc((size_t)V, sizeof *in_queue);
    if (!queue || !path_len || !in_queue) {
        free(queue);
        free(path_len);
        free(in_queue);
        return SP_ERROR;
    }

    for (int i = 0; i < V; i++) {
        dist[i] = INF;
    }
    dist[src] = 0;

    int head = 0, tail = 0, count = 0;
    queue[tail] = src;
    tail = (tail + 1) % V;
    count++;
    in_queue[src] = 1;

    SpStatus status = SP_OK;

    while (count > 0 && status == SP_OK) {
        int u = queue[head];
        head = (head + 1) % V;
        count--;
        in_queue[u] = 0;

        for (int k = g->offset[u]; k < g->offset[u + 1]; k++) {
            int v = g->dest[k];
            long long cand = dist[u] + g->weight[k]; /* dist[u] is finite here */

            if (cand < dist[v]) {
                dist[v] = cand;
                path_len[v] = path_len[u] + 1;

                /* A shortest path has at most V-1 edges; more means a cycle. */
                if (path_len[v] >= V) {
                    status = SP_NEGATIVE_CYCLE;
                    break;
                }
                if (!in_queue[v]) {
                    queue[tail] = v;
                    tail = (tail + 1) % V;
                    count++;
                    in_queue[v] = 1;
                }
            }
        }
    }

    free(queue);
    free(path_len);
    free(in_queue);
    return status;
}

int main(void)
{
    enum { V = 5 };
    static const Edge edges[] = {
        {0, 1, -1}, {0, 2,  4}, {1, 2,  3}, {1, 3,  2},
        {1, 4,  2}, {3, 2,  5}, {3, 1,  1}, {4, 3, -3}
    };
    const int E = (int)(sizeof edges / sizeof edges[0]);

    Graph *graph = graph_create(V, edges, E);
    if (graph == NULL) {
        fprintf(stderr, "Error: failed to create graph\n");
        return EXIT_FAILURE;
    }

    long long *dist = malloc((size_t)V * sizeof *dist);
    if (dist == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        graph_destroy(graph);
        return EXIT_FAILURE;
    }

    int exit_code = EXIT_SUCCESS;
    switch (shortest_paths(graph, 0, dist)) {
    case SP_OK:
        printf("Vertex\tDistance from Source\n");
        for (int i = 0; i < V; i++) {
            if (dist[i] == INF) {
                printf("%d\t\tunreachable\n", i);
            } else {
                printf("%d\t\t%lld\n", i, dist[i]);
            }
        }
        break;
    case SP_NEGATIVE_CYCLE:
        printf("Graph contains negative weight cycle\n");
        break;
    default:
        fprintf(stderr, "Error: shortest path computation failed\n");
        exit_code = EXIT_FAILURE;
        break;
    }

    free(dist);
    graph_destroy(graph);
    return exit_code;
}
