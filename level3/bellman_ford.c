#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/**
 * Queue-based Bellman-Ford (SPFA) implementation.
 *
 * The original edge-list implementation performs V-1 full scans of E edges,
 * giving O(VE) time even when distances settle much sooner. This version uses
 * adjacency lists and a work queue. It is typically faster on sparse graphs;
 * its worst-case time remains O(VE).
 */
struct Edge {
    int dest;
    int weight;
    int next;
};

struct Graph {
    int V;
    int E;
    int edge_count;
    int *head;
    struct Edge *edge;
};

struct Graph *createGraph(int V, int E) {
    if (V <= 0 || E < 0) {
        return NULL;
    }

    struct Graph *graph = malloc(sizeof(*graph));
    if (graph == NULL) {
        return NULL;
    }

    graph->V = V;
    graph->E = E;
    graph->edge_count = 0;
    graph->head = malloc((size_t)V * sizeof(*graph->head));
    graph->edge = E > 0 ? malloc((size_t)E * sizeof(*graph->edge)) : NULL;

    if (graph->head == NULL || (E > 0 && graph->edge == NULL)) {
        free(graph->edge);
        free(graph->head);
        free(graph);
        return NULL;
    }

    for (int i = 0; i < V; ++i) {
        graph->head[i] = -1;
    }
    return graph;
}

int addEdge(struct Graph *graph, int src, int dest, int weight) {
    if (graph == NULL || src < 0 || src >= graph->V ||
        dest < 0 || dest >= graph->V || graph->edge_count >= graph->E) {
        return 0;
    }

    int index = graph->edge_count++;
    graph->edge[index].dest = dest;
    graph->edge[index].weight = weight;
    graph->edge[index].next = graph->head[src];
    graph->head[src] = index;
    return 1;
}

void BellmanFord(const struct Graph *graph, int src) {
    if (graph == NULL || src < 0 || src >= graph->V) {
        fprintf(stderr, "Invalid graph or source vertex\n");
        return;
    }

    long long *dist = malloc((size_t)graph->V * sizeof(*dist));
    int *queue = malloc((size_t)graph->V * sizeof(*queue));
    int *path_edges = calloc((size_t)graph->V, sizeof(*path_edges));
    unsigned char *in_queue = calloc((size_t)graph->V, sizeof(*in_queue));

    if (dist == NULL || queue == NULL || path_edges == NULL || in_queue == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(in_queue);
        free(path_edges);
        free(queue);
        free(dist);
        return;
    }

    for (int i = 0; i < graph->V; ++i) {
        dist[i] = LLONG_MAX;
    }
    dist[src] = 0;

    int queue_head = 0;
    int queue_tail = 0;
    int queue_size = 0;
    queue[queue_tail] = src;
    queue_tail = (queue_tail + 1) % graph->V;
    ++queue_size;
    in_queue[src] = 1;

    while (queue_size > 0) {
        int u = queue[queue_head];
        queue_head = (queue_head + 1) % graph->V;
        --queue_size;
        in_queue[u] = 0;

        for (int i = graph->head[u]; i != -1; i = graph->edge[i].next) {
            int v = graph->edge[i].dest;
            long long candidate = dist[u] + graph->edge[i].weight;

            if (candidate < dist[v]) {
                dist[v] = candidate;
                path_edges[v] = path_edges[u] + 1;

                if (path_edges[v] >= graph->V) {
                    printf("Graph contains negative weight cycle\n");
                    free(in_queue);
                    free(path_edges);
                    free(queue);
                    free(dist);
                    return;
                }

                if (!in_queue[v]) {
                    queue[queue_tail] = v;
                    queue_tail = (queue_tail + 1) % graph->V;
                    ++queue_size;
                    in_queue[v] = 1;
                }
            }
        }
    }

    printf("Vertex Distance from Source\n");
    for (int i = 0; i < graph->V; ++i) {
        if (dist[i] == LLONG_MAX) {
            printf("%d \t\t %d\n", i, INT_MAX);
        } else {
            printf("%d \t\t %lld\n", i, dist[i]);
        }
    }

    free(in_queue);
    free(path_edges);
    free(queue);
    free(dist);
}

void debugGraph(const struct Graph *graph) {
    if (graph != NULL) {
        printf("DEBUG: Graph has %d vertices and %d edges.\n",
               graph->V, graph->edge_count);
    }
}

void printEdge(int src, const struct Edge *edge) {
    if (edge != NULL) {
        printf("Edge: %d -> %d (w: %d)\n", src, edge->dest, edge->weight);
    }
}

void freeGraph(struct Graph *graph) {
    if (graph != NULL) {
        free(graph->edge);
        free(graph->head);
        free(graph);
    }
}

int main(void) {
    struct Graph *graph = createGraph(5, 8);
    if (graph == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    const int edges[][3] = {
        {0, 1, -1}, {0, 2, 4}, {1, 2, 3}, {1, 3, 2},
        {1, 4, 2}, {3, 2, 5}, {3, 1, 1}, {4, 3, -3}
    };
    for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i) {
        if (!addEdge(graph, edges[i][0], edges[i][1], edges[i][2])) {
            fprintf(stderr, "Could not add graph edge\n");
            freeGraph(graph);
            return EXIT_FAILURE;
        }
    }

    BellmanFord(graph, 0);
    freeGraph(graph);
    return EXIT_SUCCESS;
}
