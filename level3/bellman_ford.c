#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Bellman-Ford is O(VE) in the worst case and supports negative weights.
 * Keeping edges in a flat array gives O(E) storage and good scan locality.
 * Early termination makes graphs that converge quickly cheaper in practice.
 */
struct Edge {
    int src, dest, weight;
};

struct Graph {
    int V, E;
    struct Edge *edge;
};

static struct Graph *createGraph(int V, int E) {
    if (V <= 0 || E < 0) return NULL;
    struct Graph *graph = malloc(sizeof(*graph));
    if (!graph) return NULL;
    graph->V = V;
    graph->E = E;
    graph->edge = E ? malloc((size_t)E * sizeof(*graph->edge)) : NULL;
    if (E && !graph->edge) {
        free(graph);
        return NULL;
    }
    return graph;
}

static int BellmanFord(const struct Graph *graph, int src) {
    if (!graph || src < 0 || src >= graph->V) return -1;
    int64_t *dist = malloc((size_t)graph->V * sizeof(*dist));
    if (!dist) return -1;

    for (int i = 0; i < graph->V; ++i) dist[i] = INT64_MAX;
    dist[src] = 0;

    for (int i = 1; i < graph->V; ++i) {
        int changed = 0;
        for (int j = 0; j < graph->E; ++j) {
            const struct Edge *e = &graph->edge[j];
            if (e->src < 0 || e->src >= graph->V || e->dest < 0 || e->dest >= graph->V) {
                free(dist);
                return -1;
            }
            if (dist[e->src] == INT64_MAX) continue;
            int64_t candidate = dist[e->src] + (int64_t)e->weight;
            if (candidate < dist[e->dest]) {
                dist[e->dest] = candidate;
                changed = 1;
            }
        }
        if (!changed) break;
    }

    for (int i = 0; i < graph->E; ++i) {
        const struct Edge *e = &graph->edge[i];
        if (dist[e->src] != INT64_MAX &&
            dist[e->src] + (int64_t)e->weight < dist[e->dest]) {
            puts("Graph contains negative weight cycle");
            free(dist);
            return 1;
        }
    }

    puts("Vertex Distance from Source");
    for (int i = 0; i < graph->V; ++i) {
        if (dist[i] == INT64_MAX)
            printf("%d\t\t unreachable\n", i);
        else
            printf("%d\t\t %" PRId64 "\n", i, dist[i]);
    }
    free(dist);
    return 0;
}

int main(void) {
    const int V = 5, E = 8;
    struct Graph *graph = createGraph(V, E);
    if (!graph) {
        fputs("Unable to allocate graph\n", stderr);
        return EXIT_FAILURE;
    }

    const struct Edge edges[] = {
        {0, 1, -1}, {0, 2, 4}, {1, 2, 3}, {1, 3, 2},
        {1, 4, 2}, {3, 2, 5}, {3, 1, 1}, {4, 3, -3}
    };
    for (int i = 0; i < E; ++i) graph->edge[i] = edges[i];

    int result = BellmanFord(graph, 0);
    free(graph->edge);
    free(graph);
    return result < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
