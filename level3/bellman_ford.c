#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Edge {
    int source;
    int destination;
    int weight;
};

struct Graph {
    int vertex_count;
    int edge_count;
    struct Edge *edges;
};

enum BellmanFordStatus {
    BELLMAN_FORD_SUCCESS,
    BELLMAN_FORD_NEGATIVE_CYCLE,
    BELLMAN_FORD_INVALID_GRAPH,
    BELLMAN_FORD_DISTANCE_OVERFLOW
};

static struct Graph *create_graph(int vertex_count, int edge_count)
{
    if (vertex_count <= 0 || edge_count < 0 ||
        (size_t)edge_count > SIZE_MAX / sizeof(struct Edge)) {
        return NULL;
    }

    struct Graph *graph = malloc(sizeof(*graph));
    if (graph == NULL) {
        return NULL;
    }

    graph->vertex_count = vertex_count;
    graph->edge_count = edge_count;
    graph->edges = NULL;

    if (edge_count > 0) {
        graph->edges = malloc((size_t)edge_count * sizeof(*graph->edges));
        if (graph->edges == NULL) {
            free(graph);
            return NULL;
        }
    }

    return graph;
}

static void destroy_graph(struct Graph *graph)
{
    if (graph != NULL) {
        free(graph->edges);
        free(graph);
    }
}

static int add_weight(long long distance, int weight, long long *result)
{
    if ((weight > 0 && distance > LLONG_MAX - weight) ||
        (weight < 0 && distance < LLONG_MIN - weight)) {
        return 0;
    }

    *result = distance + weight;
    return 1;
}

static enum BellmanFordStatus bellman_ford(
    const struct Graph *graph,
    int source,
    long long distances[])
{
    if (graph == NULL || distances == NULL ||
        graph->vertex_count <= 0 || graph->edge_count < 0 ||
        source < 0 || source >= graph->vertex_count ||
        (graph->edge_count > 0 && graph->edges == NULL)) {
        return BELLMAN_FORD_INVALID_GRAPH;
    }

    for (int edge_index = 0; edge_index < graph->edge_count; edge_index++) {
        const struct Edge *edge = &graph->edges[edge_index];
        if (edge->source < 0 || edge->source >= graph->vertex_count ||
            edge->destination < 0 || edge->destination >= graph->vertex_count) {
            return BELLMAN_FORD_INVALID_GRAPH;
        }
    }

    for (int vertex = 0; vertex < graph->vertex_count; vertex++) {
        distances[vertex] = LLONG_MAX;
    }
    distances[source] = 0;

    /*
     * Bellman-Ford supports negative weights. Its worst-case complexity remains
     * O(V * E); stopping when a pass makes no updates avoids unnecessary passes.
     */
    for (int iteration = 1; iteration < graph->vertex_count; iteration++) {
        int updated = 0;

        for (int edge_index = 0; edge_index < graph->edge_count; edge_index++) {
            const struct Edge *edge = &graph->edges[edge_index];
            if (distances[edge->source] == LLONG_MAX) {
                continue;
            }

            long long candidate_distance;
            if (!add_weight(
                    distances[edge->source],
                    edge->weight,
                    &candidate_distance)) {
                return BELLMAN_FORD_DISTANCE_OVERFLOW;
            }

            if (candidate_distance < distances[edge->destination]) {
                distances[edge->destination] = candidate_distance;
                updated = 1;
            }
        }

        if (!updated) {
            break;
        }
    }

    for (int edge_index = 0; edge_index < graph->edge_count; edge_index++) {
        const struct Edge *edge = &graph->edges[edge_index];
        if (distances[edge->source] == LLONG_MAX) {
            continue;
        }

        long long candidate_distance;
        if (!add_weight(
                distances[edge->source],
                edge->weight,
                &candidate_distance)) {
            return BELLMAN_FORD_DISTANCE_OVERFLOW;
        }

        if (candidate_distance < distances[edge->destination]) {
            return BELLMAN_FORD_NEGATIVE_CYCLE;
        }
    }

    return BELLMAN_FORD_SUCCESS;
}

int main(void)
{
    const int vertex_count = 5;
    const struct Edge initial_edges[] = {
        {0, 1, -1},
        {0, 2, 4},
        {1, 2, 3},
        {1, 3, 2},
        {1, 4, 2},
        {3, 2, 5},
        {3, 1, 1},
        {4, 3, -3}
    };
    const int edge_count =
        (int)(sizeof(initial_edges) / sizeof(initial_edges[0]));

    struct Graph *graph = create_graph(vertex_count, edge_count);
    if (graph == NULL) {
        fprintf(stderr, "Error: unable to allocate the graph.\n");
        return EXIT_FAILURE;
    }

    for (int edge_index = 0; edge_index < edge_count; edge_index++) {
        graph->edges[edge_index] = initial_edges[edge_index];
    }

    if ((size_t)vertex_count > SIZE_MAX / sizeof(long long)) {
        fprintf(stderr, "Error: distance array is too large.\n");
        destroy_graph(graph);
        return EXIT_FAILURE;
    }

    long long *distances =
        malloc((size_t)vertex_count * sizeof(*distances));
    if (distances == NULL) {
        fprintf(stderr, "Error: unable to allocate the distance array.\n");
        destroy_graph(graph);
        return EXIT_FAILURE;
    }

    enum BellmanFordStatus status = bellman_ford(graph, 0, distances);
    int exit_status = EXIT_SUCCESS;

    if (status == BELLMAN_FORD_NEGATIVE_CYCLE) {
        printf("Graph contains a negative weight cycle.\n");
    } else if (status == BELLMAN_FORD_INVALID_GRAPH) {
        fprintf(stderr, "Error: invalid graph or source vertex.\n");
        exit_status = EXIT_FAILURE;
    } else if (status == BELLMAN_FORD_DISTANCE_OVERFLOW) {
        fprintf(stderr, "Error: path distance exceeds the supported range.\n");
        exit_status = EXIT_FAILURE;
    } else {
        printf("Vertex Distance from Source\n");
        for (int vertex = 0; vertex < vertex_count; vertex++) {
            printf("%d\t\t", vertex);
            if (distances[vertex] == LLONG_MAX) {
                printf("INF\n");
            } else {
                printf("%lld\n", distances[vertex]);
            }
        }
    }

    free(distances);
    destroy_graph(graph);
    return exit_status;
}
