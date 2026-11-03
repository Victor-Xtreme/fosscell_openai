#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

/**
 * Level 3: Optimization
 * 
 * Your task:
 * 1. Analyze the performance of the current Bellman-Ford implementation.
 * 2. Stop when distances converge; retain contiguous edge storage.
 * 3. Ensure all memory is managed correctly.
 */

struct Edge {
    int src, dest, weight;
};

struct Graph {
    int V, E;
    struct Edge* edge;
};

struct Graph* createGraph(int V, int E) {
    if (V <= 0 || E < 0 || (size_t)E > SIZE_MAX / sizeof(struct Edge)) {
        return NULL;
    }
    struct Graph* graph = malloc(sizeof *graph);
    if (graph == NULL) {
        return NULL;
    }
    graph->V = V;
    graph->E = E;
    graph->edge = E == 0 ? NULL : malloc((size_t)E * sizeof *graph->edge);
    if (E != 0 && graph->edge == NULL) {
        free(graph);
        return NULL;
    }
    return graph;
}

/* Return success, including when a reachable negative cycle is reported. */
bool BellmanFord(const struct Graph* graph, int src) {
    if (graph == NULL || graph->V <= 0 || graph->E < 0 ||
        src < 0 || src >= graph->V ||
        (graph->E != 0 && graph->edge == NULL)) {
        fprintf(stderr, "Error: invalid graph or source.\n");
        return false;
    }
    int V = graph->V;
    int E = graph->E;
    for (int i = 0; i < E; i++) {
        if (graph->edge[i].src < 0 || graph->edge[i].src >= V ||
            graph->edge[i].dest < 0 || graph->edge[i].dest >= V) {
            fprintf(stderr, "Error: edge endpoint is outside the graph.\n");
            return false;
        }
    }
    if ((size_t)V > SIZE_MAX / sizeof(int64_t)) {
        fprintf(stderr, "Error: distance array is too large.\n");
        return false;
    }
    int64_t *dist = malloc((size_t)V * sizeof *dist);
    int64_t *next = malloc((size_t)V * sizeof *next);
    if (dist == NULL || next == NULL) {
        fprintf(stderr, "Error: unable to allocate distances.\n");
        free(dist);
        free(next);
        return false;
    }

    for (int i = 0; i < V; i++)
        dist[i] = INT64_MAX;
    dist[src] = 0;

    for (int i = 1; i <= V - 1; i++) {
        bool changed = false;
        memcpy(next, dist, (size_t)V * sizeof *dist);
        for (int j = 0; j < E; j++) {
            int u = graph->edge[j].src;
            int v = graph->edge[j].dest;
            int weight = graph->edge[j].weight;
            /* Each pass adds at most one edge, bounding sums to V int weights. */
            if (dist[u] != INT64_MAX && dist[u] + weight < next[v]) {
                next[v] = dist[u] + weight;
                changed = true;
            }
        }
        int64_t *temp = dist;
        dist = next;
        next = temp;
        if (!changed) {
            break;
        }
    }

    for (int i = 0; i < E; i++) {
        int u = graph->edge[i].src;
        int v = graph->edge[i].dest;
        int weight = graph->edge[i].weight;
        if (dist[u] != INT64_MAX && dist[u] + weight < dist[v]) {
            printf("Graph contains negative weight cycle\n");
            free(dist);
            free(next);
            return true;
        }
    }

    printf("Vertex Distance from Source\n");
    for (int i = 0; i < V; ++i) {
        if (dist[i] == INT64_MAX) {
            printf("%d \t\t INF\n", i);
        } else {
            printf("%d \t\t %" PRId64 "\n", i, dist[i]);
        }
    }
    free(dist);
    free(next);
    return true;
}

void debugGraph(struct Graph* g) {
    printf("DEBUG: Graph has %d vertices and %d edges.\n", g->V, g->E);
}

void printEdge(struct Edge e) {
    printf("Edge: %d -> %d (w: %d)\n", e.src, e.dest, e.weight);
}


int main(void) {
    int V = 5;
    int E = 8;
    struct Graph* graph = createGraph(V, E);
    if (graph == NULL) {
        fprintf(stderr, "Error: unable to create graph.\n");
        return EXIT_FAILURE;
    }

    graph->edge[0].src = 0; graph->edge[0].dest = 1; graph->edge[0].weight = -1;
    graph->edge[1].src = 0; graph->edge[1].dest = 2; graph->edge[1].weight = 4;
    graph->edge[2].src = 1; graph->edge[2].dest = 2; graph->edge[2].weight = 3;
    graph->edge[3].src = 1; graph->edge[3].dest = 3; graph->edge[3].weight = 2;
    graph->edge[4].src = 1; graph->edge[4].dest = 4; graph->edge[4].weight = 2;
    graph->edge[5].src = 3; graph->edge[5].dest = 2; graph->edge[5].weight = 5;
    graph->edge[6].src = 3; graph->edge[6].dest = 1; graph->edge[6].weight = 1;
    graph->edge[7].src = 4; graph->edge[7].dest = 3; graph->edge[7].weight = -3;

    bool success = BellmanFord(graph, 0);

    free(graph->edge);
    free(graph);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
