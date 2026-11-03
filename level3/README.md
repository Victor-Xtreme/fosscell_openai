# Level 3: Optimization

The task mentions `fibonacci.py` and `bellman_ford.c`. The repository initially
contained only `bellman_ford.c`; this level now includes an iterative Fibonacci
implementation as well.

## Fibonacci

[`fibonacci.py`](fibonacci.py) implements `fibonacci(n)` iteratively. The
naive recursive Fibonacci takes **O(2ⁿ)** time; this version
takes **O(n)** additions and **O(1)** auxiliary space (excluding the growing
integer result). It rejects booleans and non-integers with `TypeError`, and
negative indices with `ValueError`.

```python
from fibonacci import fibonacci

print(fibonacci(10))  # 55
```

## Bellman–Ford

Bellman–Ford finds shortest paths in a weighted directed graph, including
graphs with negative edge weights. An edge-list implementation has worst-case
time complexity **O(VE)** and uses **O(V)** distance storage; it is not
exponential. A Fibonacci heap is suited to Dijkstra's algorithm and does not
remove Bellman–Ford's need to account for negative edges and cycles.

[`bellman_ford.c`](bellman_ford.c) stops relaxing edges as soon as a complete
pass makes no updates. This preserves the **O(VE)** worst-case bound while
reducing work when distances settle early. It checks for reachable negative
weight cycles, reports unreachable vertices, uses 64-bit distances with
overflow checks, validates edge endpoints, and frees allocated memory.

Compile with a C99 or newer compiler:

```sh
cc -std=c99 -Wall -Wextra -Wpedantic bellman_ford.c -o bellman_ford
./bellman_ford
```

For the included graph and source vertex `0`, the finite distances are `0, -1,
2, -2, 1` for vertices `0` through `4`; no reachable negative cycle is present.
