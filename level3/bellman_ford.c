#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

/* O(VE) worst-case time, O(V) auxiliary space.
 * Stop early when a full relaxation pass makes no changes.
 * A contiguous edge list provides efficient sequential traversal.
 */

struct Edge {
    int src, dest, weight;
};

struct Graph {
    int V, E;
    struct Edge* edge;
};

struct Graph* createGraph(int V, int E) {
    if (V <= 0 || E < 0 ||
        (size_t)E > SIZE_MAX / sizeof(struct Edge))
        return NULL;

    struct Graph* graph = malloc(sizeof *graph);
    if (graph == NULL)
        return NULL;

    graph->V = V;
    graph->E = E;
    graph->edge = E == 0 ? NULL : malloc((size_t)E * sizeof *graph->edge);
    if (E != 0 && graph->edge == NULL) {
        free(graph);
        return NULL;
    }
    return graph;
}

/* Return failure rather than allowing signed integer overflow.
 * LLONG_MAX is reserved for unreachable vertices. */
static int add_weight(long long distance, int weight, long long* result) {
    if ((weight > 0 && distance >= LLONG_MAX - weight) ||
        (weight < 0 && distance < LLONG_MIN - weight))
        return 0;
    *result = distance + weight;
    return 1;
}

int BellmanFord(const struct Graph* graph, int src) {
    if (graph == NULL || graph->V <= 0 || graph->E < 0 ||
        src < 0 || src >= graph->V ||
        (graph->E > 0 && graph->edge == NULL))
        return EXIT_FAILURE;

    const int V = graph->V;
    const int E = graph->E;
    for (int j = 0; j < E; ++j) {
        if (graph->edge[j].src < 0 || graph->edge[j].src >= V ||
            graph->edge[j].dest < 0 || graph->edge[j].dest >= V)
            return EXIT_FAILURE;
    }

    if ((size_t)V > SIZE_MAX / sizeof(long long))
        return EXIT_FAILURE;
    long long* dist = malloc((size_t)V * sizeof *dist);
    if (dist == NULL)
        return EXIT_FAILURE;

    for (int i = 0; i < V; ++i)
        dist[i] = LLONG_MAX;
    dist[src] = 0;

    /* The Vth pass detects a negative cycle reachable from src.
     * Early convergence makes a separate cycle scan unnecessary. */
    for (int pass = 0; pass < V; ++pass) {
        int changed = 0;
        for (int j = 0; j < E; ++j) {
            const struct Edge* edge = &graph->edge[j];
            if (dist[edge->src] == LLONG_MAX)
                continue;
            long long candidate;
            if (!add_weight(dist[edge->src], edge->weight, &candidate)) {
                fprintf(stderr, "Distance exceeds supported range.\n");
                free(dist);
                return EXIT_FAILURE;
            }
            if (candidate < dist[edge->dest]) {
                if (pass == V - 1) {
                    printf("Graph contains negative weight cycle\n");
                    free(dist);
                    return EXIT_FAILURE;
                }
                dist[edge->dest] = candidate;
                changed = 1;
            }
        }
        if (!changed)
            break;
    }

    printf("Vertex Distance from Source\n");
    for (int i = 0; i < V; ++i) {
        if (dist[i] == LLONG_MAX)
            printf("%d \t\t INF\n", i);
        else
            printf("%d \t\t %lld\n", i, dist[i]);
    }
    free(dist);
    return EXIT_SUCCESS;
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
        fprintf(stderr, "Graph allocation failed.\n");
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

    int status = BellmanFord(graph, 0);

    free(graph->edge);
    free(graph);

    return status;
}
