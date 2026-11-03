# Level 3: Optimization

The task is to optimize `fibonacci.py` and `bellman_ford.c`. The original naive
recursive Fibonacci approach takes exponential time. The Bellman-Ford
implementation takes O(VE) time in the worst case; it cannot be replaced by a
Fibonacci-heap shortest-path algorithm because the graph may contain negative
weights.

- Fibonacci now uses an iterative O(n)-time, O(1)-space algorithm.
- Bellman-Ford keeps its O(VE) worst-case bound and O(V + E) storage, but stops
  early when an iteration makes no distance changes. The flat edge array gives
  efficient sequential scans, and 64-bit distances avoid ordinary `int`
  overflow on path sums.
- The Bellman-Ford routine reports reachable negative-weight cycles and
  unreachable vertices, and releases its allocated distance array on every
  return path.
