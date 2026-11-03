#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

struct Edge {
    int src, dest, weight;
};

struct Graph {
    int V, E;
    struct Edge* edge;
};

struct Graph* createGraph(int V, int E) {
    struct Graph* graph = malloc(sizeof(struct Graph));
    if (!graph) return NULL;
    graph->V = V;
    graph->E = E;
    graph->edge = malloc((size_t)E * sizeof(struct Edge));
    if (!graph->edge) {
        free(graph);
        return NULL;
    }
    return graph;
}

void freeGraph(struct Graph* graph) {
    if (!graph) return;
    free(graph->edge);
    free(graph);
}

/* Returns 0 on success, 1 on negative cycle, -1 on error. */
int BellmanFord(struct Graph* graph, int src) {
    int V = graph->V, E = graph->E;
    int status = -1;

    if (src < 0 || src >= V) return -1;

    long long *dist = malloc((size_t)V * sizeof(long long));
    int *start      = calloc((size_t)V + 1, sizeof(int));   /* CSR row offsets */
    int *adjDest    = malloc((size_t)E * sizeof(int));
    int *adjWeight  = malloc((size_t)E * sizeof(int));
    int *pathLen    = calloc((size_t)V, sizeof(int));       /* edges on current best path */
    char *inQueue   = calloc((size_t)V, sizeof(char));
    int *queue      = malloc((size_t)(V + 1) * sizeof(int)); /* circular queue */

    if (!dist || !start || !adjDest || !adjWeight || !pathLen || !inQueue || !queue)
        goto cleanup;

    /* Build CSR adjacency (counting sort by source vertex) */
    for (int i = 0; i < E; i++) {
        int u = graph->edge[i].src, v = graph->edge[i].dest;
        if (u < 0 || u >= V || v < 0 || v >= V) goto cleanup;
        start[u + 1]++;
    }
    for (int i = 0; i < V; i++)
        start[i + 1] += start[i];

    int *pos = malloc((size_t)V * sizeof(int));
    if (!pos) goto cleanup;
    for (int i = 0; i < V; i++) pos[i] = start[i];
    for (int i = 0; i < E; i++) {
        int p = pos[graph->edge[i].src]++;
        adjDest[p]   = graph->edge[i].dest;
        adjWeight[p] = graph->edge[i].weight;
    }
    free(pos);

    for (int i = 0; i < V; i++) dist[i] = LLONG_MAX;
    dist[src] = 0;

    int head = 0, tail = 0, cap = V + 1;
    queue[tail] = src;
    tail = (tail + 1) % cap;
    inQueue[src] = 1;

    status = 0;
    while (head != tail && status == 0) {
        int u = queue[head];
        head = (head + 1) % cap;
        inQueue[u] = 0;

        for (int k = start[u]; k < start[u + 1]; k++) {
            int v = adjDest[k];
            long long nd = dist[u] + adjWeight[k];
            if (nd < dist[v]) {
                dist[v] = nd;
                pathLen[v] = pathLen[u] + 1;
                if (pathLen[v] >= V) {      /* path with >= V edges => negative cycle */
                    status = 1;
                    break;
                }
                if (!inQueue[v]) {
                    queue[tail] = v;
                    tail = (tail + 1) % cap;
                    inQueue[v] = 1;
                }
            }
        }
    }

    if (status == 1) {
        printf("Graph contains negative weight cycle\n");
    } else if (status == 0) {
        printf("Vertex Distance from Source\n");
        for (int i = 0; i < V; i++) {
            if (dist[i] == LLONG_MAX)
                printf("%d \t\t INF\n", i);
            else
                printf("%d \t\t %lld\n", i, dist[i]);
        }
    }

cleanup:
    free(dist);
    free(start);
    free(adjDest);
    free(adjWeight);
    free(pathLen);
    free(inQueue);
    free(queue);
    if (status == -1)
        fprintf(stderr, "BellmanFord: allocation failure or invalid input\n");
    return status;
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
    if (!graph) {
        fprintf(stderr, "Failed to allocate graph\n");
        return EXIT_FAILURE;
    }

    graph->edge[0] = (struct Edge){0, 1, -1};
    graph->edge[1] = (struct Edge){0, 2,  4};
    graph->edge[2] = (struct Edge){1, 2,  3};
    graph->edge[3] = (struct Edge){1, 3,  2};
    graph->edge[4] = (struct Edge){1, 4,  2};
    graph->edge[5] = (struct Edge){3, 2,  5};
    graph->edge[6] = (struct Edge){3, 1,  1};
    graph->edge[7] = (struct Edge){4, 3, -3};

    BellmanFord(graph, 0);

    freeGraph(graph);
    return 0;
}