#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/**
 * Optimized Bellman-Ford Algorithm with Early Termination
 */

struct Edge {
    int src, dest, weight;
};

struct Graph {
    int V, E;
    struct Edge* edge;
};

struct Graph* createGraph(int V, int E) {
    struct Graph* graph = (struct Graph*)malloc(sizeof(struct Graph));
    if (!graph) return NULL;
    
    graph->V = V;
    graph->E = E;
    graph->edge = (struct Edge*)malloc(graph->E * sizeof(struct Edge));
    if (!graph->edge) {
        free(graph);
        return NULL;
    }
    return graph;
}

void freeGraph(struct Graph* graph) {
    if (graph) {
        if (graph->edge) {
            free(graph->edge);
        }
        free(graph);
    }
}

void BellmanFord(struct Graph* graph, int src) {
    int V = graph->V;
    int E = graph->E;
    
    // Heap-allocate distance array to handle large V safely
    int* dist = (int*)malloc(V * sizeof(int));
    if (!dist) {
        fprintf(stderr, "Memory allocation failed for dist array.\n");
        return;
    }

    for (int i = 0; i < V; i++)
        dist[i] = INT_MAX;
    dist[src] = 0;

    // Relax edges up to V - 1 times with Early Termination
    for (int i = 1; i <= V - 1; i++) {
        int updated = 0;
        for (int j = 0; j < E; j++) {
            int u = graph->edge[j].src;
            int v = graph->edge[j].dest;
            int weight = graph->edge[j].weight;
            
            if (dist[u] != INT_MAX && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                updated = 1;
            }
        }
        // Early termination: if no edge was relaxed, shortest paths are finalized
        if (!updated) break;
    }

    // Negative cycle check
    for (int i = 0; i < E; i++) {
        int u = graph->edge[i].src;
        int v = graph->edge[i].dest;
        int weight = graph->edge[i].weight;
        
        if (dist[u] != INT_MAX && dist[u] + weight < dist[v]) {
            printf("Graph contains negative weight cycle\n");
            free(dist);
            return;
        }
    }

    printf("Vertex Distance from Source\n");
    for (int i = 0; i < V; ++i) {
        if (dist[i] == INT_MAX)
            printf("%d \t\t INF\n", i);
        else
            printf("%d \t\t %d\n", i, dist[i]);
    }

    free(dist);
}

int main() {
    int V = 5;
    int E = 8;
    struct Graph* graph = createGraph(V, E);
    if (!graph) return 1;

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
