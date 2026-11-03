#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#define INF LLONG_MAX

typedef struct {
    int src;
    int dest;
    int weight;
} Edge;

typedef struct {
    int V;
    int E;
    Edge *edge;
} Graph;

// Create graph safely
Graph *create_graph(int V, int E) {
    if (V <= 0 || E < 0) {
        return NULL;
    }

    if ((size_t)E > SIZE_MAX / sizeof(Edge)) {
        return NULL;
    }

    Graph *graph = malloc(sizeof(Graph));

    if (graph == NULL) {
        return NULL;
    }

    graph->V = V;
    graph->E = E;
    graph->edge = NULL;

    if (E > 0) {
        graph->edge = malloc((size_t)E * sizeof(Edge));

        if (graph->edge == NULL) {
            free(graph);
            return NULL;
        }
    }

    return graph;
}

// Free graph memory
void free_graph(Graph *graph) {
    if (graph != NULL) {
        free(graph->edge);
        free(graph);
    }
}

// Bellman-Ford shortest path algorithm
int bellman_ford(const Graph *graph, int src) {
    if (graph == NULL || src < 0 || src >= graph->V) {
        fprintf(stderr, "Invalid graph or source vertex.\n");
        return 0;
    }

    int V = graph->V;
    int E = graph->E;

    // Validate edge endpoints
    for (int i = 0; i < E; i++) {
        if (graph->edge[i].src < 0 ||
            graph->edge[i].src >= V ||
            graph->edge[i].dest < 0 ||
            graph->edge[i].dest >= V) {

            fprintf(stderr, "Invalid edge detected.\n");
            return 0;
        }
    }

    // Allocate distance array
    long long *dist = malloc((size_t)V * sizeof(*dist));

    if (dist == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 0;
    }

    // Initialize distances
    for (int i = 0; i < V; i++) {
        dist[i] = INF;
    }

    dist[src] = 0;

    // Relax edges
    for (int i = 0; i < V - 1; i++) {
        int updated = 0;

        for (int j = 0; j < E; j++) {
            int u = graph->edge[j].src;
            int v = graph->edge[j].dest;
            int weight = graph->edge[j].weight;

            if (dist[u] != INF &&
                dist[u] + weight < dist[v]) {

                dist[v] = dist[u] + weight;
                updated = 1;
            }
        }

        // Optimization: stop if no distance changed
        if (!updated) {
            break;
        }
    }

    // Detect reachable negative-weight cycles
    for (int i = 0; i < E; i++) {
        int u = graph->edge[i].src;
        int v = graph->edge[i].dest;
        int weight = graph->edge[i].weight;

        if (dist[u] != INF &&
            dist[u] + weight < dist[v]) {

            printf("Graph contains a reachable negative-weight cycle.\n");
            free(dist);
            return 0;
        }
    }

    // Print shortest distances
    printf("\nVertex\tDistance from Source\n");

    for (int i = 0; i < V; i++) {
        if (dist[i] == INF) {
            printf("%d\tINF\n", i);
        } else {
            printf("%d\t%lld\n", i, dist[i]);
        }
    }

    free(dist);
    return 1;
}

int main(void) {
    int V = 5;
    int E = 8;

    Graph *graph = create_graph(V, E);

    if (graph == NULL) {
        fprintf(stderr, "Graph creation failed.\n");
        return EXIT_FAILURE;
    }

    Edge edges[] = {
        {0, 1, -1},
        {0, 2, 4},
        {1, 2, 3},
        {1, 3, 2},
        {1, 4, 2},
        {3, 2, 5},
        {3, 1, 1},
        {4, 3, -3}
    };

    for (int i = 0; i < E; i++) {
        graph->edge[i] = edges[i];
    }

    bellman_ford(graph, 0);

    free_graph(graph);

    return EXIT_SUCCESS;
}
