
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>

#define INF INT64_MAX

struct Edge {
    int src;
    int dest;
    int weight;
};

struct Graph {
    int V;
    int E;
    struct Edge *edge;
};

enum BFStatus {
    BF_ERROR = -1,
    BF_SUCCESS = 0,
    BF_NEGATIVE_CYCLE = 1
};

/* Safely adds an integer edge weight to an int64_t distance.
   INT64_MAX is reserved as the INF value. */
static int safeAdd(int64_t a, int b, int64_t *result) {
    int64_t weight = (int64_t)b;

    if (weight > 0 && a > INT64_MAX - weight)
        return 0;

    if (weight < 0 && a < INT64_MIN - weight)
        return 0;

    *result = a + weight;

    if (*result == INF)
        return 0;

    return 1;
}

/* Creates a graph with allocation checks. */
struct Graph *createGraph(int V, int E) {
    if (V <= 0 || E < 0) {
        fprintf(stderr, "Error: Invalid graph dimensions.\n");
        return NULL;
    }

    if ((size_t)E > SIZE_MAX / sizeof(struct Edge)) {
        fprintf(stderr, "Error: Edge allocation size is too large.\n");
        return NULL;
    }

    struct Graph *graph = malloc(sizeof(struct Graph));

    if (graph == NULL) {
        fprintf(stderr, "Error: Graph allocation failed.\n");
        return NULL;
    }

    graph->V = V;
    graph->E = E;
    graph->edge = NULL;

    if (E > 0) {
        graph->edge = malloc((size_t)E * sizeof(struct Edge));

        if (graph->edge == NULL) {
            fprintf(stderr, "Error: Edge allocation failed.\n");
            free(graph);
            return NULL;
        }
    }

    return graph;
}

/* Frees all graph memory. */
void freeGraph(struct Graph *graph) {
    if (graph != NULL) {
        free(graph->edge);
        free(graph);
    }
}

/* Prints an individual edge. */
void printEdge(const struct Edge *edge) {
    printf("%d -> %d (weight: %d)\n",
           edge->src, edge->dest, edge->weight);
}

/* Displays the graph for debugging. */
void debugGraph(const struct Graph *graph) {
    if (graph == NULL)
        return;

    printf("DEBUG: Graph has %d vertices and %d edges.\n",
           graph->V, graph->E);

    for (int i = 0; i < graph->E; i++)
        printEdge(&graph->edge[i]);
}

/* Bellman-Ford shortest path algorithm. */
enum BFStatus BellmanFord(const struct Graph *graph, int src) {
    if (graph == NULL || src < 0 || src >= graph->V) {
        fprintf(stderr, "Error: Invalid graph or source vertex.\n");
        return BF_ERROR;
    }

    int V = graph->V;
    int E = graph->E;

    /* Validate all edges before processing. */
    for (int i = 0; i < E; i++) {
        int u = graph->edge[i].src;
        int v = graph->edge[i].dest;

        if (u < 0 || u >= V || v < 0 || v >= V) {
            fprintf(stderr, "Error: Invalid edge at index %d.\n", i);
            return BF_ERROR;
        }
    }

    /* Allocate distance array dynamically. */
    if ((size_t)V > SIZE_MAX / sizeof(int64_t)) {
        fprintf(stderr, "Error: Distance array is too large.\n");
        return BF_ERROR;
    }

    int64_t *dist = malloc((size_t)V * sizeof(int64_t));

    if (dist == NULL) {
        fprintf(stderr, "Error: Distance array allocation failed.\n");
        return BF_ERROR;
    }

    /* Initialize distances. */
    for (int i = 0; i < V; i++)
        dist[i] = INF;

    dist[src] = 0;

    /* Relax all edges up to V-1 times. */
    for (int i = 1; i < V; i++) {
        int updated = 0;

        for (int j = 0; j < E; j++) {
            int u = graph->edge[j].src;
            int v = graph->edge[j].dest;
            int weight = graph->edge[j].weight;
            int64_t candidate;

            if (dist[u] == INF)
                continue;

            if (!safeAdd(dist[u], weight, &candidate)) {
                fprintf(stderr,
                        "Error: Distance arithmetic overflow.\n");
                free(dist);
                return BF_ERROR;
            }

            if (candidate < dist[v]) {
                dist[v] = candidate;
                updated = 1;
            }
        }

        /* Optimization: stop if no distance was updated. */
        if (!updated)
            break;
    }

    /* Check for reachable negative-weight cycles. */
    for (int i = 0; i < E; i++) {
        int u = graph->edge[i].src;
        int v = graph->edge[i].dest;
        int weight = graph->edge[i].weight;
        int64_t candidate;

        if (dist[u] == INF)
            continue;

        if (!safeAdd(dist[u], weight, &candidate)) {
            fprintf(stderr,
                    "Error: Distance arithmetic overflow.\n");
            free(dist);
            return BF_ERROR;
        }

        if (candidate < dist[v]) {
            printf("Graph contains a reachable negative weight cycle.\n");
            free(dist);
            return BF_NEGATIVE_CYCLE;
        }
    }

    /* Display shortest distances. */
    printf("\nVertex\tDistance from Source %d\n", src);

    for (int i = 0; i < V; i++) {
        printf("%d\t", i);

        if (dist[i] == INF)
            printf("UNREACHABLE\n");
        else
            printf("%" PRId64 "\n", dist[i]);
    }

    free(dist);
    return BF_SUCCESS;
}

int main(void) {
    int V = 5;
    int E = 8;

    struct Graph *graph = createGraph(V, E);

    if (graph == NULL)
        return EXIT_FAILURE;

    /* Sample graph edges. */
    struct Edge edges[] = {
        {0, 1, -1},
        {0, 2,  4},
        {1, 2,  3},
        {1, 3,  2},
        {1, 4,  2},
        {3, 2,  5},
        {3, 1,  1},
        {4, 3, -3}
    };

    for (int i = 0; i < E; i++)
        graph->edge[i] = edges[i];

    debugGraph(graph);

    enum BFStatus result = BellmanFord(graph, 0);

    freeGraph(graph);

    if (result == BF_ERROR)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
