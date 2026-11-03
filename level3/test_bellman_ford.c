/* Compile this file alone; it includes the implementation under test. */
#define main bellman_ford_demo_main
#include "bellman_ford.c"
#undef main
#include <assert.h>

static enum PathResult reference(const struct Graph *graph, int src,
                                 int64_t distances[]) {
    for (int i = 0; i < graph->V; i++) {
        distances[i] = INT64_MAX;
    }
    distances[src] = 0;
    for (int round = 0; round < graph->V - 1; round++) {
        for (int i = 0; i < graph->E; i++) {
            const struct Edge *edge = &graph->edge[i];
            if (distances[edge->src] != INT64_MAX &&
                distances[edge->src] + edge->weight < distances[edge->dest]) {
                distances[edge->dest] = distances[edge->src] + edge->weight;
            }
        }
    }
    for (int i = 0; i < graph->E; i++) {
        const struct Edge *edge = &graph->edge[i];
        if (distances[edge->src] != INT64_MAX &&
            distances[edge->src] + edge->weight < distances[edge->dest]) {
            return PATH_NEGATIVE_CYCLE;
        }
    }
    return PATH_OK;
}

static void compare(const struct Graph *graph, int source) {
    int64_t actual[8], expected[8];
    enum PathResult result = shortest_paths(graph, source, actual);
    assert(result == reference(graph, source, expected));
    if (result == PATH_OK) {
        for (int i = 0; i < graph->V; i++) {
            assert(actual[i] == expected[i]);
        }
    }
}

int main(void) {
    int64_t distances[8];
    struct Graph empty = {1, 0, NULL};
    assert(shortest_paths(&empty, 0, distances) == PATH_OK);
    assert(distances[0] == 0);
    assert(shortest_paths(&empty, -1, distances) == PATH_INVALID_GRAPH);
    assert(shortest_paths(NULL, 0, distances) == PATH_INVALID_GRAPH);
    assert(createGraph(0, 0) == NULL);
    assert(createGraph(1, -1) == NULL);

    struct Edge special[] = {{0, 1, INT_MAX}, {1, 2, INT_MAX}, {3, 3, -1}};
    struct Graph graph = {4, 3, special};
    compare(&graph, 0); /* Unreachable negative cycle and wide distances. */
    assert(shortest_paths(&graph, 0, distances) == PATH_OK);
    assert(distances[2] == 2 * (int64_t)INT_MAX);
    assert(distances[3] == INT64_MAX);
    compare(&graph, 3); /* Reachable negative self-loop. */
    special[0].weight = INT_MIN;
    special[1].weight = INT_MIN;
    compare(&graph, 0);
    special[0].dest = 4;
    assert(shortest_paths(&graph, 0, distances) == PATH_INVALID_GRAPH);

    srand(42);
    for (int trial = 0; trial < 2000; trial++) {
        struct Edge edges[64];
        int vertices = 1 + rand() % 8;
        int edge_count = rand() % 65;
        for (int i = 0; i < edge_count; i++) {
            edges[i] = (struct Edge){rand() % vertices, rand() % vertices,
                                     rand() % 21 - 10};
        }
        struct Graph random_graph = {vertices, edge_count, edges};
        for (int source = 0; source < vertices; source++) {
            compare(&random_graph, source);
        }
    }
    puts("Bellman-Ford edge cases and 2000 random graph comparisons passed.");
    return EXIT_SUCCESS;
}
