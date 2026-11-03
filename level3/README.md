# Level 3: Optimization

The available implementation is `bellman_ford.c`.

## Optimization

Bellman-Ford has worst-case time complexity **O(VE)**, where `V` is the number
of vertices and `E` is the number of edges. The original implementation stores
all edges in a flat array and scans every edge on every pass.

The implementation now:

- Stores outgoing edges in adjacency lists, avoiding scans of unrelated edges.
- Stops relaxation early when a complete pass makes no changes.
- Checks for integer overflow during distance calculations.
- Checks allocation and edge-insertion failures.
- Frees every graph and distance allocation.

The algorithm remains Bellman-Ford because negative edge weights must be
supported; a standard Dijkstra or Fibonacci-heap implementation would not be
correct for this input.
