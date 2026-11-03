# Level 3: Optimization

Your task is to optimize the provided code in `fibonacci.py` and `bellman_ford.c`.
The current implementation works but is extremely inefficient for larger inputs (exponential time complexity).

- Analyze the time complexity.
- Refactor the code to improve performance (e.g., using memoization or an iterative approach).

## Implemented optimization

This directory supplies `bellman_ford.c` only; there is no `fibonacci.py` to
optimize. The original Bellman–Ford algorithm takes O(VE) time, not exponential
time, and scans all edges V times including negative-cycle detection.

The updated implementation stops relaxation when a pass changes no distances.
For paths requiring k relaxation passes, processing takes O(V + (k + 1)(V + E))
time, with k at most V - 1. Worst-case time is O(V(V + E)); when E is at least
V, this is O(VE). A stable graph finishes after one pass and a cycle check,
taking O(V + E). Sparse or disconnected graphs still incur O(V) per pass for
copying distance buffers. Graph storage uses O(V + E) memory overall.

Contiguous edge storage is retained for sequential scans. A Fibonacci heap is
not a direct optimization for Bellman–Ford, and replacing it with Dijkstra
would lose support for the negative edges used by the example.

Two heap-allocated `int64_t` buffers replace the stack-allocated integer array.
Each pass reads previous distances and writes the next distances, so paths
gain at most one edge per pass. With 32-bit `int` vertices and weights, even the
final cycle-check addition fits in 64 bits. This avoids overflow from ordinary
32-bit path sums and repeated in-place relaxation around negative cycles.
Allocation failures, graph dimensions, source indices, and edge endpoints are
checked; all allocated buffers are freed on every return path.

`BellmanFord()` now returns `bool`: `false` indicates invalid input or allocation
failure, and `true` means processing completed, including reporting a reachable
negative cycle. Only cycles reachable from the source are reported. Unreachable
vertices print `INF`; finite distances print as 64-bit integers.

## Build and run

Requires a C11 compiler with 32-bit `int` and standard 64-bit integer support:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror bellman_ford.c -o bellman_ford
./bellman_ford
```

The sample graph produces distances `0, -1, 2, -2, 1` from source vertex 0.

Run the regression checks with Python 3 and GCC on PATH:

```sh
python test_bellman_ford.py
```
