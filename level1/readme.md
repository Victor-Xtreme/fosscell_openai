This script summarizes requests.log into summary.txt. For each hour and endpoint, it reports the request count, p50/p95/p99 latency, number of 5xx responses, and 5xx rate in parts per million.

It has a fast path that parses byte chunks and uses multiple processes, plus a reference implementation using Python’s datetime and sorting routines. If the fast path detects unsupported input or encounters an error, it retries with fewer processes or falls back to the reference implementation.
Main concerns
- The fast path does not fully validate timestamps. _ts_ok() checks only a handful of positions and the final Z. A malformed timestamp can pass those checks, then be grouped using its first 13 bytes instead of being parsed as a datetime. The reference implementation would reject it or handle it differently.
- Whitespace handling differs. The fast path uses buf.split(), which treats any whitespace as a separator. The reference uses .split(" "), which splits only on literal spaces. Inputs with tabs or repeated spaces may therefore be accepted and summarized differently by the fast path.
- The fast path assumes UTC timestamps. It accepts timestamps ending in Z; the reference also accepts numeric offsets. Offset timestamps trigger fallback only if the sampled timestamps fail _ts_ok(). If inputs mix timestamp formats, the sampling could miss an unsupported one.
- The multiprocessing task queue relies on pipe behavior. The parent writes all task IDs before starting workers, assuming the pipe can hold up to 32 KiB. If that write blocks because the pipe is smaller, workers have not started yet to drain it. Also, workers assume each os.read(..., 4) returns exactly four bytes, which is not guaranteed by the pipe API.
- There is a portability limitation. The fast path uses os.fork() and os.pread(), so it is Unix-oriented and will not run as written on Windows. Its broad exception handling may hide that failure by attempting the reference path instead.
- Large or negative latency values force a slower path. Latencies are stored in unsigned 32-bit arrays. Negative values and values above the limit trigger fallback; the reference path can parse larger integers, though negative values may still be accepted there.
What looks sound
For well-formed, space-separated logs with UTC Z timestamps and nonnegative latencies that fit in 32 bits, the percentile ranks use the same ceiling-based definition as the reference implementation. The grouped error counts and output ordering also appear consistent under those assumptions.

Overall: this is a performance-focused implementation with a sensible reference fallback, but the fast path’s input checks are not strong enough to guarantee equivalent results for every file the reference parser accepts. I’d validate every record’s timestamp and field format before using the fast path, and make the worker queue robust to partial reads and pipe capacity.

    
7:30 PM
