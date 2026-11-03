# Request Log Data Processor

## 1. Overview

`data_processor.py` is a command-line batch processor for request logs. It groups requests by hour and endpoint and writes a summary containing request counts, latency percentiles, server-error counts, and errors per million requests. It uses a parallelized fast path for supported input and falls back to a reference implementation when necessary.

**Implementation note:** the current `data_processor.py` does not define a `DataProcessor` class. It does not implement numerical dataset ingestion, filtering, linear transformations, or CSV persistence. Its input is a request log and its output is a whitespace-delimited summary file. This README documents the code that is present.

## 2. Requirements & Setup

- Python 3.7 or later on a Unix-like system with `os.fork` and `os.pread` available. The parallel fast path depends on Unix process support.
- Standard library modules used include `gc`, `marshal`, `os`, `sys`, `array`, `bisect`, `itertools`, and (in the fallback) `datetime`; the histogram optimization imports `collections.Counter` when needed.
- No third-party packages are required.

Run with the default directories (`/data` and `/out`):

```bash
python data_processor.py
```

Or pass directories explicitly:

```bash
python data_processor.py --data ./data --out ./out
```

The input and output basenames are fixed: `requests.log` and `summary.txt`. Import the module to call `main(data_dir, out_dir)` from Python:

```python
import data_processor

data_processor.main("./data", "./out")
```

## 3. Input and Output

Each input record has five whitespace-separated fields:

```text
timestamp method endpoint status latency_us
```

Example:

```text
2025-01-15T10:12:30.123456Z GET /api/items 200 1250
```

The timestamp format expected by the reference parser is `%Y-%m-%dT%H:%M:%S.%f%z`: it includes fractional seconds and a timezone. Status and latency must parse as integers. Latencies are in microseconds.

The output has one line per hour and endpoint, ordered by the grouping key:

```text
YYYY-MM-DDTHH:00:00Z endpoint count p50 p95 p99 errors errors_per_million
```

- `count`: number of requests in the hour/endpoint group.
- `p50`, `p95`, `p99`: nearest-rank latency percentiles. For percentile `p` and `n` values, the rank is `ceil(p * n / 100)`; the value at that one-based rank in sorted order is reported.
- `errors`: number of status codes from 500 through 599, inclusive.
- `errors_per_million`: integer floor of `errors * 1_000_000 / count`.

## 4. API Reference

There are no `DataProcessor` methods in the current source. The supported entry points are module-level functions:

### `main(data_dir, out_dir)`

Runs the processing workflow.

- `data_dir` (`str` or path-like): directory containing `requests.log`.
- `out_dir` (`str` or path-like): destination directory for `summary.txt`; created if needed.
- Returns: `None`.
- Errors: input/output filesystem errors or invalid input errors can propagate if the reference retry also fails. `main` disables garbage collection for the duration of its processing and does not restore its previous state.

### `fast_main(data_dir, out_dir, single=False)`

Runs the optimized implementation, optionally in one process.

- `data_dir`, `out_dir`: same meaning as in `main`.
- `single` (`bool`, default `False`): when true, avoid multiprocessing in both processing phases.
- Returns: `None`.
- Errors: may raise `Fallback` when input assumptions fail, `WorkerFailed` when a child process fails, or operating-system, conversion, and memory errors.

### `reference_main(data_dir, out_dir)`

Runs the reference parser and summary calculation directly.

- `data_dir`, `out_dir`: same meaning as in `main`.
- Returns: `None`.
- Errors: file access errors; `ValueError` for malformed field counts or invalid integer/timestamp values; `OverflowError` is possible for invalid numeric values elsewhere in processing.

### `cpu_budget()`

Returns the worker count selected for parallel processing, caching the result for subsequent calls. `A_INGEST_WORKERS`, when set to a non-empty value, overrides CPU detection; its value must be convertible to an integer. Otherwise the function considers available CPUs and, where available, a cgroup CPU quota.

### Internal helpers

The module also defines parsing, percentile, process, and worker helpers such as `parse_chunk`, `process_range`, `ranks_of`, `line_bounds`, `spawn`, and `collect`. They support the implementation and are not intended as stable application-facing APIs. `Fallback` and `WorkerFailed` are internal control-flow exceptions.

## 5. Examples

### Prepare a sample log and process it

```bash
mkdir -p ./data ./out
cat > ./data/requests.log <<'EOF'
2025-01-15T10:12:30.123456Z GET /api/items 200 1000
2025-01-15T10:25:00.000000Z GET /api/items 500 3000
2025-01-15T10:45:10.500000Z POST /api/items 201 2000
2025-01-15T11:02:00.000000Z GET /api/items 503 4000
EOF

python data_processor.py --data ./data --out ./out
cat ./out/summary.txt
```

The records above produce two hour/endpoint groups. The `10` hour group has three requests, one server error (`500`), and latencies `1000`, `2000`, and `3000`; the `11` hour group has one request and one server error (`503`).

### Call it from Python

```python
import data_processor

data_processor.main("./data", "./out")
```

### Limit worker count

Set `A_INGEST_WORKERS` to a positive integer to cap the number of workers selected by the program:

```bash
A_INGEST_WORKERS=2 python data_processor.py --data ./data --out ./out
```

`A_INGEST_CHUNK` controls the fast path's ingest chunk size in bytes. It must be an integer; its default is 262144 bytes. Invalid values can prevent the module from importing.

## 6. Error Handling & Edge Cases

- **Empty input file:** produces an empty `summary.txt`.
- **Input layout:** the fast parser expects exactly five fields per record. If the fast path's assumptions are not met, it switches to the reference parser. The reference parser splits on literal spaces and requires exactly five resulting fields, so repeated spaces can make a line invalid.
- **Invalid timestamp/status/latency:** the reference parser raises the relevant `datetime` or integer-conversion exception. It requires fractional seconds and an explicit timezone in the timestamp.
- **Fallback behavior:** `main` tries the parallel fast path first. If it raises `Fallback`, `main` runs the reference implementation. For another exception, it retries the fast path in single-process mode; if that also fails, it runs the reference implementation. Errors from that last attempt propagate to the caller.
- **File changes during processing:** a short read in the fast path triggers fallback, since the input may have changed while it was being read.
- **File and directory errors:** missing/unreadable input, unwritable output, or failure to create the output directory can raise operating-system exceptions.
- **Timezone display:** the output timestamp is formatted with a literal `Z`. The reference parser accepts timezone offsets, but output does not preserve the parsed offset; use UTC input timestamps ending in `Z` for unambiguous output.

