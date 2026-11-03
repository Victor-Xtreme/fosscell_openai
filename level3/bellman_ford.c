#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

struct Graph *createGraph(int V, int E) {
    if (V <= 0 || E < 0) {
        return NULL;
    }

    struct Graph *graph = malloc(sizeof *graph);
    if (graph == NULL) {
        return NULL;
    }

    graph->V = V;
    graph->E = E;
    graph->edge = E == 0 ? NULL : malloc((size_t)E * sizeof *graph->edge);
    if (E > 0 && graph->edge == NULL) {
        free(graph);
        return NULL;
    }
    return graph;
}

/*
 * Bellman-Ford runs in O(VE) time and O(V) auxiliary space. The early exit
 * makes graphs that converge quickly cheaper; negative edges remain supported.
 * A heap-based Dijkstra replacement would be incorrect for this input model.
 */
int BellmanFord(const struct Graph *graph, int src) {
    if (graph == NULL || src < 0 || src >= graph->V) {
        fprintf(stderr, "Invalid graph or source vertex\n");
        return 0;
    }

    const int V = graph->V;
    const int E = graph->E;
    const long long INF = LLONG_MAX;
    long long *dist = malloc((size_t)V * sizeof *dist);
    if (dist == NULL) {
        fprintf(stderr, "Unable to allocate distance array\n");
        return 0;
    }

    for (int i = 0; i < V; ++i) {
        dist[i] = INF;
    }
    dist[src] = 0;

    for (int i = 1; i < V; ++i) {
        int changed = 0;
        for (int j = 0; j < E; ++j) {
            const struct Edge *edge = &graph->edge[j];
            if (edge->src < 0 || edge->src >= V ||
                edge->dest < 0 || edge->dest >= V) {
                fprintf(stderr, "Edge %d has an invalid vertex\n", j);
                free(dist);
                return 0;
            }
            if (dist[edge->src] == INF) {
                continue;
            }

            /* Widen before addition to avoid signed-int overflow. */
            long long candidate = dist[edge->src] + (long long)edge->weight;
            if (candidate < dist[edge->dest]) {
                dist[edge->dest] = candidate;
                changed = 1;
            }
        }
        if (!changed) {
            break;
        }
    }

    for (int i = 0; i < E; ++i) {
        const struct Edge *edge = &graph->edge[i];
        if (edge->src < 0 || edge->src >= V ||
            edge->dest < 0 || edge->dest >= V) {
            fprintf(stderr, "Edge %d has an invalid vertex\n", i);
            free(dist);
            return 0;
        }
        if (dist[edge->src] != INF &&
            dist[edge->src] + (long long)edge->weight < dist[edge->dest]) {
            printf("Graph contains negative weight cycle\n");
            free(dist);
            return 1;
        }
    }

    printf("Vertex Distance from Source\n");
    for (int i = 0; i < V; ++i) {
        if (dist[i] == INF) {
            printf("%d\t\t unreachable\n", i);
        } else {
            printf("%d\t\t %lld\n", i, dist[i]);
        }
    }

    free(dist);
    return 1;
}

void debugGraph(const struct Graph *graph) {
    if (graph != NULL) {
        printf("DEBUG: Graph has %d vertices and %d edges.\n", graph->V, graph->E);
    }
}

void printEdge(struct Edge edge) {
    printf("Edge: %d -> %d (w: %d)\n", edge.src, edge.dest, edge.weight);
}

int main(void) {
    const int V = 5;
    const int E = 8;
    struct Graph *graph = createGraph(V, E);
    if (graph == NULL) {
        fprintf(stderr, "Unable to allocate graph\n");
        return EXIT_FAILURE;
    }

    graph->edge[0] = (struct Edge){0, 1, -1};
    graph->edge[1] = (struct Edge){0, 2, 4};
    graph->edge[2] = (struct Edge){1, 2, 3};
    graph->edge[3] = (struct Edge){1, 3, 2};
    graph->edge[4] = (struct Edge){1, 4, 2};
    graph->edge[5] = (struct Edge){3, 2, 5};
    graph->edge[6] = (struct Edge){3, 1, 1};
    graph->edge[7] = (struct Edge){4, 3, -3};

    int ok = BellmanFord(graph, 0);
    free(graph->edge);
    free(graph);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
