#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Level 3: Optimization
 *
 * Bellman-Ford remains O(VE), but the graph is stored as adjacency lists so
 * each relaxation pass visits only the edges that leave each vertex. The
 * implementation also stops early when a pass makes no changes.
 */

struct Edge {
    int dest;
    int weight;
    struct Edge *next;
};

struct Graph {
    int vertex_count;
    struct Edge **adjacency;
};

static struct Graph *create_graph(int vertex_count) {
    if (vertex_count <= 0) {
        return NULL;
    }

    struct Graph *graph = malloc(sizeof(*graph));
    if (graph == NULL) {
        return NULL;
    }

    graph->vertex_count = vertex_count;
    graph->adjacency = calloc((size_t)vertex_count, sizeof(*graph->adjacency));
    if (graph->adjacency == NULL) {
        free(graph);
        return NULL;
    }

    return graph;
}

static int add_edge(struct Graph *graph, int src, int dest, int weight) {
    if (graph == NULL || src < 0 || src >= graph->vertex_count ||
        dest < 0 || dest >= graph->vertex_count) {
        return 0;
    }

    struct Edge *edge = malloc(sizeof(*edge));
    if (edge == NULL) {
        return 0;
    }

    edge->dest = dest;
    edge->weight = weight;
    edge->next = graph->adjacency[src];
    graph->adjacency[src] = edge;
    return 1;
}

static void destroy_graph(struct Graph *graph) {
    if (graph == NULL) {
        return;
    }

    for (int vertex = 0; vertex < graph->vertex_count; vertex++) {
        struct Edge *edge = graph->adjacency[vertex];
        while (edge != NULL) {
            struct Edge *next = edge->next;
            free(edge);
            edge = next;
        }
    }

    free(graph->adjacency);
    free(graph);
}

static int safe_add_distance(long long distance, int weight, long long *result) {
    if ((weight > 0 && distance > LLONG_MAX - weight) ||
        (weight < 0 && distance < LLONG_MIN - weight)) {
        return 0;
    }

    *result = distance + weight;
    return 1;
}

static int bellman_ford(const struct Graph *graph, int source) {
    long long *distance = malloc((size_t)graph->vertex_count * sizeof(*distance));
    if (distance == NULL) {
        fprintf(stderr, "Failed to allocate distance array.\n");
        return 0;
    }

    for (int vertex = 0; vertex < graph->vertex_count; vertex++) {
        distance[vertex] = LLONG_MAX;
    }
    distance[source] = 0;

    for (int pass = 0; pass < graph->vertex_count - 1; pass++) {
        int changed = 0;

        for (int vertex = 0; vertex < graph->vertex_count; vertex++) {
            if (distance[vertex] == LLONG_MAX) {
                continue;
            }

            for (const struct Edge *edge = graph->adjacency[vertex];
                 edge != NULL; edge = edge->next) {
                long long candidate;
                if (safe_add_distance(distance[vertex], edge->weight, &candidate) &&
                    candidate < distance[edge->dest]) {
                    distance[edge->dest] = candidate;
                    changed = 1;
                }
            }
        }

        if (!changed) {
            break;
        }
    }

    for (int vertex = 0; vertex < graph->vertex_count; vertex++) {
        if (distance[vertex] == LLONG_MAX) {
            continue;
        }

        for (const struct Edge *edge = graph->adjacency[vertex];
             edge != NULL; edge = edge->next) {
            long long candidate;
            if (safe_add_distance(distance[vertex], edge->weight, &candidate) &&
                candidate < distance[edge->dest]) {
                printf("Graph contains a negative weight cycle.\n");
                free(distance);
                return 0;
            }
        }
    }

    printf("Vertex Distance from Source\n");
    for (int vertex = 0; vertex < graph->vertex_count; vertex++) {
        if (distance[vertex] == LLONG_MAX) {
            printf("%d\t\tINF\n", vertex);
        } else {
            printf("%d\t\t%lld\n", vertex, distance[vertex]);
        }
    }

    free(distance);
    return 1;
}

int main(void) {
    struct Graph *graph = create_graph(5);
    if (graph == NULL) {
        fprintf(stderr, "Failed to create graph.\n");
        return EXIT_FAILURE;
    }

    const int edges[][3] = {
        {0, 1, -1}, {0, 2, 4},  {1, 2, 3}, {1, 3, 2},
        {1, 4, 2},  {3, 2, 5},  {3, 1, 1}, {4, 3, -3}
    };
    const size_t edge_count = sizeof(edges) / sizeof(edges[0]);

    for (size_t i = 0; i < edge_count; i++) {
        if (!add_edge(graph, edges[i][0], edges[i][1], edges[i][2])) {
            fprintf(stderr, "Failed to add graph edge.\n");
            destroy_graph(graph);
            return EXIT_FAILURE;
        }
    }

    int result = bellman_ford(graph, 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    destroy_graph(graph);
    return result;
}
