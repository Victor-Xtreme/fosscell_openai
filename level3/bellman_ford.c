#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

_Static_assert(INT_MAX <= 2147483647, "Distances require at most 32-bit int weights.");

struct Edge {
    int src, dest, weight;
};

struct Graph {
    int V, E;
    struct Edge *edge;
};

enum PathResult {
    PATH_OK,
    PATH_NEGATIVE_CYCLE,
    PATH_INVALID_GRAPH,
    PATH_NO_MEMORY
};

struct Graph *createGraph(int V, int E) {
    if (V <= 0 || E < 0 || (size_t)E > SIZE_MAX / sizeof(struct Edge)) {
        return NULL;
    }
    struct Graph *graph = malloc(sizeof *graph);
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

/*
 * Index outgoing edges once, then relax only vertices changed in the previous
 * round. Snapshot frontier distances so each round adds at most one edge to a
 * path. Worst-case time: O(VE + V); auxiliary space: O(V + E).
 * INT64_MAX denotes unreachable vertices. Negative cycles must be reachable
 * from src to be reported. Distances are usable only when PATH_OK is returned.
 */
enum PathResult shortest_paths(const struct Graph *graph, int src,
                               int64_t distances[]) {
    if (graph == NULL || distances == NULL || graph->V <= 0 || graph->E < 0 ||
        src < 0 || src >= graph->V || (graph->E > 0 && graph->edge == NULL)) {
        return PATH_INVALID_GRAPH;
    }
    size_t vertices = (size_t)graph->V;
    size_t edges = (size_t)graph->E;
    if (vertices > SIZE_MAX / sizeof(int64_t) ||
        vertices > SIZE_MAX / sizeof(int) || edges > SIZE_MAX / sizeof(int)) {
        return PATH_NO_MEMORY;
    }
    for (int i = 0; i < graph->E; i++) {
        if (graph->edge[i].src < 0 || graph->edge[i].src >= graph->V ||
            graph->edge[i].dest < 0 || graph->edge[i].dest >= graph->V) {
            return PATH_INVALID_GRAPH;
        }
    }

    int *head = malloc(vertices * sizeof *head);
    int *next = edges == 0 ? NULL : malloc(edges * sizeof *next);
    int *frontier = malloc(vertices * sizeof *frontier);
    int *pending = malloc(vertices * sizeof *pending);
    int64_t *snapshot = malloc(vertices * sizeof *snapshot);
    unsigned char *queued = calloc(vertices, sizeof *queued);
    enum PathResult result = PATH_NO_MEMORY;
    if (head == NULL || (edges != 0 && next == NULL) || frontier == NULL ||
        pending == NULL || snapshot == NULL || queued == NULL) {
        goto cleanup;
    }
    for (int i = 0; i < graph->V; i++) {
        head[i] = -1;
        distances[i] = INT64_MAX;
    }
    for (int i = 0; i < graph->E; i++) {
        int source = graph->edge[i].src;
        next[i] = head[source];
        head[source] = i;
    }
    distances[src] = 0;
    frontier[0] = src;
    int count = 1;
    for (int round = 0; round < graph->V - 1 && count != 0; round++) {
        for (int i = 0; i < count; i++) {
            snapshot[i] = distances[frontier[i]];
        }
        int pending_count = 0;
        for (int i = 0; i < count; i++) {
            int source = frontier[i];
            for (int j = head[source]; j != -1; j = next[j]) {
                const struct Edge *edge = &graph->edge[j];
                /* At most V int-weight edges are summed, fitting int64_t
                 * on systems with 32-bit int (including the target platform). */
                int64_t candidate = snapshot[i] + (int64_t)edge->weight;
                if (candidate < distances[edge->dest]) {
                    distances[edge->dest] = candidate;
                    if (!queued[edge->dest]) {
                        queued[edge->dest] = 1;
                        pending[pending_count++] = edge->dest;
                    }
                }
            }
        }
        for (int i = 0; i < pending_count; i++) {
            queued[pending[i]] = 0;
        }
        int *temporary = frontier;
        frontier = pending;
        pending = temporary;
        count = pending_count;
    }

    result = PATH_OK;
    for (int i = 0; i < graph->E; i++) {
        const struct Edge *edge = &graph->edge[i];
        if (distances[edge->src] != INT64_MAX &&
            distances[edge->src] + (int64_t)edge->weight < distances[edge->dest]) {
            result = PATH_NEGATIVE_CYCLE;
            break;
        }
    }

cleanup:
    free(queued);
    free(snapshot);
    free(pending);
    free(frontier);
    free(next);
    free(head);
    return result;
}

void BellmanFord(struct Graph *graph, int src) {
    if (graph == NULL || graph->V <= 0 ||
        (size_t)graph->V > SIZE_MAX / sizeof(int64_t)) {
        fprintf(stderr, "Invalid graph.\n");
        return;
    }
    int64_t *distances = malloc((size_t)graph->V * sizeof *distances);
    if (distances == NULL) {
        fprintf(stderr, "Unable to allocate distances.\n");
        return;
    }
    enum PathResult result = shortest_paths(graph, src, distances);
    if (result == PATH_NEGATIVE_CYCLE) {
        printf("Graph contains negative weight cycle\n");
    } else if (result != PATH_OK) {
        fprintf(stderr, "Unable to compute paths: %s.\n",
                result == PATH_NO_MEMORY ? "allocation failed" : "invalid graph");
    } else {
        printf("Vertex Distance from Source\n");
        for (int i = 0; i < graph->V; i++) {
            if (distances[i] == INT64_MAX) {
                printf("%d \t\t INF\n", i);
            } else {
                printf("%d \t\t %" PRId64 "\n", i, distances[i]);
            }
        }
    }
    free(distances);
}

void debugGraph(struct Graph *graph) {
    if (graph != NULL) {
        printf("DEBUG: Graph has %d vertices and %d edges.\n", graph->V, graph->E);
    }
}

void printEdge(struct Edge edge) {
    printf("Edge: %d -> %d (w: %d)\n", edge.src, edge.dest, edge.weight);
}

int main(void) {
    const struct Edge edges[] = {
        {0, 1, -1}, {0, 2, 4}, {1, 2, 3}, {1, 3, 2},
        {1, 4, 2}, {3, 2, 5}, {3, 1, 1}, {4, 3, -3}
    };
    const int edge_count = (int)(sizeof edges / sizeof edges[0]);
    struct Graph *graph = createGraph(5, edge_count);
    if (graph == NULL) {
        fprintf(stderr, "Unable to allocate graph.\n");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < edge_count; i++) {
        graph->edge[i] = edges[i];
    }
    BellmanFord(graph, 0);
    free(graph->edge);
    free(graph);
    return EXIT_SUCCESS;
}
