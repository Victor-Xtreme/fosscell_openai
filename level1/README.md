# Request Log Summarizer

Reads `requests.log` and writes `summary.txt` with per-hour, per-endpoint latency and error statistics.

## Purpose

This module ingests a large, line-oriented request log and condenses it into a compact statistical summary. For every `(hour, endpoint)` group it reports the request count, latency percentiles (p50, p95, p99), the number of 5xx errors, and the error rate in parts per million.

### Problem it solves

Raw request logs can run to many gigabytes, which makes them impractical to inspect directly or to process with a naive Python loop. This module addresses that by:

- **Ingestion:** reading the file in cache-sized chunks, in parallel across CPU cores, with minimal per-line overhead.
- **Statistical analysis:** computing exact nearest-rank percentiles and error rates per group, including groups too large to sort in one pass.
- **Grouping/filtering:** bucketing records by hour and endpoint, and classifying statuses 500-599 as errors.
- **Transformation:** converting timestamps to hour labels and latencies to compact unsigned integers to reduce memory use.
- **Persistence:** writing a sorted, deterministic plain-text summary (`summary.txt`) that downstream tools can consume.

Correctness is preserved by a slower reference implementation that takes over whenever the input doesn't match the fast path's assumptions.

## Usage

### Run from the command line

```bash
python a_ingest_fast.py [--data DATA_DIR] [--out OUT_DIR]
```

### Import and call from another script

```python
from a_ingest_fast import main

main(data_dir="/path/to/input", out_dir="/path/to/output")
```

You can also call the two inner entry points directly:

```python
from a_ingest_fast import fast_main, reference_main

# High-performance parallel path (Unix only)
fast_main(data_dir="/data", out_dir="/out")

# Safe fallback — pure Python, no forking
reference_main(data_dir="/data", out_dir="/out")
```

| Option   | Default | Description                                  |
|----------|---------|----------------------------------------------|
| `--data` | `/data` | Directory containing `requests.log`          |
| `--out`  | `/out`  | Directory where `summary.txt` is written     |

## Input format

One request per line, five space-separated fields:

```
<timestamp> <method> <endpoint> <status> <latency_us>
```

Example:

```
2026-01-15T10:23:45.123456Z GET /api/users 200 1532
```

## Output format

One line per `(hour, endpoint)` pair, sorted by hour then endpoint:

```
<hour>:00:00Z <endpoint> <count> <p50> <p95> <p99> <5xx_count> <errors_per_million>
```

- Percentiles use the nearest-rank method: `ceil(p * n / 100)`.
- Errors are statuses 500-599.
- `errors_per_million = errors * 1_000_000 // count`.

## Configuration

| Environment variable | Default  | Description                           |
|----------------------|----------|---------------------------------------|
| `A_INGEST_WORKERS`   | CPU count (cgroup-aware) | Number of worker processes |
| `A_INGEST_CHUNK`     | `262144` | Bytes read per chunk                  |

## Public API

---

### `parse_chunk(buf, acc)`

Parses one buffer of whole log lines and merges the results into an accumulator.

| Parameter | Type   | Description                                              |
|-----------|--------|----------------------------------------------------------|
| `buf`     | `bytes` | A buffer containing complete newline-terminated log lines |
| `acc`     | `dict`  | Accumulator `{(hour, endpoint): [error_count, array]}` modified in place |

**Returns:** `None`

**Raises:**
- `Fallback` — if the token count doesn't match `5 * line_count`, timestamps fail spot checks, latency values overflow `uint32`, or latency values are not integers.

---

### `process_range(fd, a, b, acc)`

Reads and parses all complete lines in the byte range `[a, b)` of an open file descriptor, in `CHUNK`-sized reads.

| Parameter | Type   | Description                                      |
|-----------|--------|--------------------------------------------------|
| `fd`      | `int`  | Open file descriptor (read-only)                 |
| `a`       | `int`  | Start byte offset (inclusive)                    |
| `b`       | `int`  | End byte offset (exclusive)                      |
| `acc`     | `dict` | Accumulator passed through to `parse_chunk`      |

**Returns:** `None`

**Raises:**
- `Fallback` — if the file size changes during reading, or if `parse_chunk` raises `Fallback`.

---

### `cpu_budget()`

Returns the number of worker processes to use, respecting `A_INGEST_WORKERS`, CPU affinity, and cgroup CPU quotas.

**Returns:** `int` — number of logical CPUs available (always ≥ 1).

**Raises:** nothing.

---

### `line_bounds(fd, size, ntasks)`

Computes `ntasks + 1` byte offsets that divide the file into line-aligned ranges.

| Parameter | Type  | Description                                     |
|-----------|-------|-------------------------------------------------|
| `fd`      | `int` | Open file descriptor                            |
| `size`    | `int` | Total file size in bytes                        |
| `ntasks`  | `int` | Number of ranges to produce                     |

**Returns:** `list[int]` of length `ntasks + 1`. The first element is `0`, the last is `size`, and every inner element is the byte offset of the first character of a line.

**Raises:** nothing directly; `os.pread` may raise `OSError`.

---

### `spawn(fn)`

Forks a child process that calls `fn()` and writes the returned bytes to a pipe.

| Parameter | Type       | Description                                            |
|-----------|------------|--------------------------------------------------------|
| `fn`      | `callable` | A zero-argument function that returns a `bytes` object |

**Returns:** `tuple[int, int]` — `(pid, read_fd)` where `pid` is the child's process ID and `read_fd` is the readable end of the pipe.

**Raises:** `OSError` — if `os.fork()` or `os.pipe()` fails.

Child exit codes:
| Code | Meaning           |
|------|-------------------|
| `0`  | Success           |
| `1`  | Unhandled exception |
| `3`  | `Fallback` raised |

---

### `collect(children)`

Reads output from all child processes created by `spawn` and waits for them to exit.

| Parameter  | Type                  | Description                                |
|------------|-----------------------|--------------------------------------------|
| `children` | `list[tuple[int,int]]`| List of `(pid, read_fd)` pairs from `spawn`|

**Returns:** `list[bytes]` — one blob per child, in the same order as `children`.

**Raises:**
- `Fallback` — if any child exited with code `3`.
- `WorkerFailed` — if any child exited with a non-zero code other than `3`.

---

### `ranks_of(pieces)`

Computes `n`, `p50`, `p95`, and `p99` (nearest-rank) for a group of latency values.

| Parameter | Type            | Description                                           |
|-----------|-----------------|-------------------------------------------------------|
| `pieces`  | `list[bytes]`   | List of raw `uint32` buffers (from `array("I").tobytes()`) |

**Returns:** `tuple[int, int, int, int]` — `(n, p50, p95, p99)` where `n` is the total count and the percentiles are in microseconds.

**Raises:** nothing directly; internally falls back between sort, histogram, and multi-run bisection strategies automatically.

---

### `fast_main(data_dir, out_dir, single=False)`

High-performance entry point: parallel parsing and percentile computation.

| Parameter  | Type   | Description                                                |
|------------|--------|------------------------------------------------------------|
| `data_dir` | `str`  | Directory containing `requests.log`                        |
| `out_dir`  | `str`  | Directory where `summary.txt` will be written              |
| `single`   | `bool` | If `True`, disables forking (single-process mode). Default: `False` |

**Returns:** `None`

**Raises:**
- `Fallback` — propagated from parsing if the input is malformed.
- `WorkerFailed` — if a child process dies unexpectedly.
- `OSError` — on file or fork errors.

---

### `reference_main(data_dir, out_dir)`

Slow, correct fallback: processes the log line by line using `datetime.strptime`.

| Parameter  | Type  | Description                                   |
|------------|-------|-----------------------------------------------|
| `data_dir` | `str` | Directory containing `requests.log`           |
| `out_dir`  | `str` | Directory where `summary.txt` will be written |

**Returns:** `None`

**Raises:**
- `ValueError` — if a line has the wrong number of fields, an unparseable timestamp, or a non-integer status or latency.
- `OSError` — if the input file cannot be opened or the output directory cannot be created.

---

### `main(data_dir, out_dir)`

Top-level entry point with automatic fallback and error recovery.

| Parameter  | Type  | Description                                   |
|------------|-------|-----------------------------------------------|
| `data_dir` | `str` | Directory containing `requests.log`           |
| `out_dir`  | `str` | Directory where `summary.txt` will be written |

**Returns:** `None`

**Raises:** nothing — all exceptions are caught internally. Execution order:
1. `fast_main` (parallel)
2. `reference_main` on `Fallback`
3. `fast_main(single=True)` on any other exception
4. `reference_main` if single-process fast path also fails

---

## How it works

1. **Fast path:** splits the file into line-aligned ranges, parses them in parallel forked workers, then computes percentiles in parallel across groups.
2. **Fallback:** if the input doesn't match the fast path's assumptions (e.g. unexpected timestamp format or non-integer latency), the straightforward reference algorithm runs instead.
3. **Failure recovery:** on fork or memory errors it retries single-process, then falls back to the reference algorithm.

## Requirements

- Python 3.7+
- Unix-like OS (uses `os.fork` and `os.pread`)
- No third-party dependencies

### Standard library dependencies

| Module    | Purpose                                                      |
|-----------|--------------------------------------------------------------|
| `math`    | Numerical computations and percentile rank calculations      |
| `csv`     | Reading and writing CSV files for persistent storage         |
| `typing`  | Type hints for function signatures and data structures       |
| `os`      | File I/O (`pread`, `fork`), CPU detection, and path helpers  |
| `array`   | Compact unsigned-int buffers for latency values              |
| `bisect`  | Binary search over sorted runs during percentile computation |
| `marshal` | Fast serialisation of worker results across forked processes |
| `gc`      | Garbage-collection control to reduce pause times             |
| `sys`     | Argument parsing and exit codes                              |
| `itertools` | `compress` for selecting error rows efficiently            |