# Level 3: Bellman–Ford Optimization

## Algorithm and complexity

Bellman–Ford finds shortest paths in a weighted directed graph, including graphs with negative edge weights. The edge-list implementation has worst-case time complexity **O(VE)** and uses **O(V)** distance storage. A Fibonacci heap is suited to Dijkstra's algorithm; it does not remove Bellman–Ford's need to account for negative edges and cycles.

The implementation in [`bellman_ford.c`](bellman_ford.c) stops relaxing edges as soon as a complete pass makes no updates. This preserves the **O(VE)** worst-case bound while reducing work on graphs whose distances settle early. It checks for reachable negative-weight cycles after relaxation.

The program also checks allocation and graph input bounds, uses 64-bit distances with overflow checks, reports unreachable vertices, and frees allocated memory on all algorithm paths.

## Build and run

Compile with a C99 or newer compiler:

```sh
cc -std=c99 -Wall -Wextra -Wpedantic bellman_ford.c -o bellman_ford
./bellman_ford
```

This directory currently contains `bellman_ford.c`; the `fibonacci.py` named in the original task description is not present here.
