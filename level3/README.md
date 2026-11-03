# Level 3: Optimization

Your task is to optimize the provided code in `fibonacci.py` and `bellman_ford.c`.
The current implementation works but is extremely inefficient for larger inputs (exponential time complexity).

- Analyze the time complexity.
- Refactor the code to improve performance (e.g., using memoization or an iterative approach).

## Bellman–Ford optimization

The original implementation takes O(VE) time, rather than exponential time:
all E edges are scanned in each of V − 1 rounds, followed by a negative-cycle
check. Its distance array takes O(V) space.

The updated implementation builds an outgoing-edge index and processes only
vertices whose distances changed in the preceding round. It stops when no
vertices change. Worst-case time remains O(VE + V), with O(V + E) auxiliary
space, but inactive and unreachable vertices no longer cause repeated edge
relaxations. Frontier distances are snapshotted to keep each round bounded to
one additional edge per path. A final edge scan detects reachable negative
cycles; unreachable negative cycles do not invalidate source distances.

A Fibonacci heap is not appropriate here: negative weights are supported, so
Dijkstra's greedy shortest-path algorithm cannot replace Bellman–Ford.
Distances use `int64_t`, with `INT64_MAX` for unreachable vertices (printed as
`INF`). Compilation requires at most 32-bit `int` weights, ensuring sums of up
to V edges fit the distance type. Allocations and vertex indices are checked,
and all temporary allocations are freed on success and failure.

`shortest_paths(graph, source, distances)` returns `PATH_OK`,
`PATH_NEGATIVE_CYCLE`, `PATH_INVALID_GRAPH`, or `PATH_NO_MEMORY`. The caller
provides space for V `int64_t` distances, usable only on `PATH_OK`.
The original `BellmanFord` printing interface remains available.

Build and run from the repository root:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic level3/bellman_ford.c -o /tmp/bellman_ford
/tmp/bellman_ford
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined level3/test_bellman_ford.c -o /tmp/test_bellman_ford
/tmp/test_bellman_ford
```

The tests compare against a conventional Bellman–Ford reference across 2,000
random graphs and cover negative cycles, disconnected vertices, invalid inputs,
and distances exceeding the range of `int`.

The README mentions `fibonacci.py`, but no such file is supplied in this clone.
