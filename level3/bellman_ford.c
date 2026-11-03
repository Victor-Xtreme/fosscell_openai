        }
    }

    /* A further improvement proves a reachable negative-weight cycle. */
    for (int i = 0; i < edge_count; ++i) {
        const Edge *edge = &edges[i];
        long long candidate;

        if (distances[edge->from] == LLONG_MAX) {
            continue;
        }

        if (!safe_add(distances[edge->from], edge->weight, &candidate)) {
            fprintf(stderr, "Error: distance overflow during cycle detection.\n");
            return -1;
        }

        if (candidate < distances[edge->to]) {
            return 1;
        }
    }

    return 0;
}

int main(void) {
    int vertex_count;
    int edge_count;
    int source;
    Edge *edges = NULL;
    long long *distances = NULL;
    int exit_code = EXIT_FAILURE;

    /* Input format: V E, source, then E lines of: from to weight. */
    printf("Enter number of vertices and edges: ");
    if (scanf("%d %d", &vertex_count, &edge_count) != 2 ||
        vertex_count <= 0 || edge_count < 0) {
        fprintf(stderr, "Error: expected a positive vertex count and a non-negative edge count.\n");
        return EXIT_FAILURE;
    }

    printf("Enter source vertex (0 to %d): ", vertex_count - 1);
    if (scanf("%d", &source) != 1 || source < 0 || source >= vertex_count) {
        fprintf(stderr, "Error: invalid source vertex.\n");
        return EXIT_FAILURE;
    }

    if ((size_t)vertex_count > SIZE_MAX / sizeof(*distances) ||
        (size_t)edge_count > SIZE_MAX / sizeof(*edges)) {
        fprintf(stderr, "Error: requested graph is too large.\n");
        return EXIT_FAILURE;
    }

    distances = malloc((size_t)vertex_count * sizeof(*distances));
    if (distances == NULL) {
        fprintf(stderr, "Error: could not allocate distance array.\n");
        goto cleanup;
    }

    if (edge_count > 0) {
        edges = malloc((size_t)edge_count * sizeof(*edges));
        if (edges == NULL) {
            fprintf(stderr, "Error: could not allocate edge array.\n");
            goto cleanup;
        }
    }

    for (int i = 0; i < edge_count; ++i) {
        printf("Enter edge %d (from to weight): ", i + 1);
        if (scanf("%d %d %lld", &edges[i].from, &edges[i].to,
                  &edges[i].weight) != 3) {
            fprintf(stderr, "Error: invalid edge input.\n");
            goto cleanup;
        }

        if (edges[i].from < 0 || edges[i].from >= vertex_count ||
            edges[i].to < 0 || edges[i].to >= vertex_count) {
            fprintf(stderr, "Error: edge endpoint is outside the valid vertex range.\n");
            goto cleanup;
        }
    }

    {
        int result = bellman_ford(vertex_count, edge_count, edges,
                                  source, distances);

        if (result < 0) {
            goto cleanup;
        }
        if (result > 0) {
            printf("A negative-weight cycle is reachable from the source.\n");
            exit_code = EXIT_SUCCESS;
            goto cleanup;
        }
    }

    printf("\nShortest distances from vertex %d:\n", source);
    for (int i = 0; i < vertex_count; ++i) {
        if (distances[i] == LLONG_MAX) {
            printf("Vertex %d: INF (unreachable)\n", i);
        } else {
            printf("Vertex %d: %lld\n", i, distances[i]);
        }
    }

    exit_code = EXIT_SUCCESS;

cleanup:
    free(edges);
    free(distances);
    return exit_code;
}
