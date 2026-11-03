#include <inttypes.h>
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

static struct Graph *create_graph(int vertex_count, int edge_count) {
    if (vertex_count <= 0 || edge_count < 0) {
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

static void free_graph(struct Graph *graph) {
    if (graph != NULL) {
        free(graph->edges);
        free(graph);
    }
}

/* Returns 0 on success, 1 for a reachable negative cycle, and -1 on error. */
static int bellman_ford(const struct Graph *graph, int source) {
    if (graph == NULL || source < 0 || source >= graph->vertex_count) {
        fprintf(stderr, "Error: invalid graph or source vertex.\n");
        return -1;
    }
    for (int edge_index = 0; edge_index < graph->edge_count; edge_index++) {
        const struct Edge *edge = &graph->edges[edge_index];
        if (edge->source < 0 || edge->source >= graph->vertex_count ||
            edge->destination < 0 || edge->destination >= graph->vertex_count) {
            fprintf(stderr, "Error: edge endpoint is outside the graph.\n");
            return -1;
        }
    }

    int64_t *distances = malloc((size_t)graph->vertex_count * sizeof(*distances));
    if (distances == NULL) {
        fprintf(stderr, "Error: unable to allocate distance array.\n");
        return -1;
    }

    for (int vertex = 0; vertex < graph->vertex_count; vertex++) {
        distances[vertex] = INT64_MAX;
    }
    distances[source] = 0;

    /* Stop early when a full pass makes no changes. */
    for (int pass = 1; pass < graph->vertex_count; pass++) {
        int updated = 0;
        for (int edge_index = 0; edge_index < graph->edge_count; edge_index++) {
            const struct Edge *edge = &graph->edges[edge_index];
            if (distances[edge->source] == INT64_MAX) {
                continue;
            }

            int64_t weight = edge->weight;
            if ((weight > 0 && distances[edge->source] > INT64_MAX - weight) ||
                (weight < 0 && distances[edge->source] < INT64_MIN - weight)) {
                fprintf(stderr, "Error: path distance exceeds the supported range.\n");
                free(distances);
                return -1;
            }

            int64_t candidate = distances[edge->source] + weight;
            if (candidate < distances[edge->destination]) {
                distances[edge->destination] = candidate;
                updated = 1;
            }
        }
        if (!updated) {
            break;
        }
    }

    for (int edge_index = 0; edge_index < graph->edge_count; edge_index++) {
        const struct Edge *edge = &graph->edges[edge_index];
        if (distances[edge->source] == INT64_MAX) {
            continue;
        }
        int64_t weight = edge->weight;
        if ((weight > 0 && distances[edge->source] > INT64_MAX - weight) ||
            (weight < 0 && distances[edge->source] < INT64_MIN - weight)) {
            fprintf(stderr, "Error: path distance exceeds the supported range.\n");
            free(distances);
            return -1;
        }
        if (distances[edge->source] + weight < distances[edge->destination]) {
            printf("Graph contains a reachable negative weight cycle.\n");
            free(distances);
            return 1;
        }
    }

    printf("Vertex Distance from Source\n");
    for (int vertex = 0; vertex < graph->vertex_count; vertex++) {
        if (distances[vertex] == INT64_MAX) {
            printf("%d\t\t unreachable\n", vertex);
        } else {
            printf("%d\t\t %" PRId64 "\n", vertex, distances[vertex]);
        }
    }

    free(distances);
    return 0;
}

int main(void) {
    const int vertex_count = 5;
    const int edge_count = 8;
    struct Graph *graph = create_graph(vertex_count, edge_count);
    if (graph == NULL) {
        fprintf(stderr, "Error: unable to create graph.\n");
        return EXIT_FAILURE;
    }

    const struct Edge input_edges[] = {
        {0, 1, -1}, {0, 2, 4}, {1, 2, 3}, {1, 3, 2},
        {1, 4, 2}, {3, 2, 5}, {3, 1, 1}, {4, 3, -3}
    };
    for (int index = 0; index < edge_count; index++) {
        graph->edges[index] = input_edges[index];
    }

    int result = bellman_ford(graph, 0);
    free_graph(graph);
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
