# Data Processor

> **Implementation note:** The current `level1/data_processor.py` does **not** define a `DataProcessor` class or the numerical-dataset API described in the original task brief. It implements a command-line log-ingestion and aggregation pipeline. This README documents the code that is actually present rather than describing methods that do not exist.

## 1. Overview

`data_processor.py` processes application request logs and writes hourly endpoint-level latency and error summaries.

For each `(hour, endpoint)` group, it calculates:

- Request count.
- 50th, 95th, and 99th percentile latency in microseconds.
- Number of HTTP 5xx responses (status codes 500–599).
- Error rate in parts per million (PPM), calculated using integer division.

The implementation is designed for large input files. Its fast path reads whole-line byte ranges in chunks, aggregates data by hour and endpoint, and can use multiple worker processes. It uses optimized sorting/rank-selection techniques for percentile calculation. If the input does not match assumptions made by the fast parser, it falls back to a reference implementation. If the parallel fast path fails, it retries the fast path in single-process mode before using the reference implementation.

This is a log-processing pipeline, not a general-purpose numerical dataset library: it does not expose a `DataProcessor` class, arbitrary statistical methods, filtering, linear transformations, or CSV import/export.

## 2. Requirements & Setup

### Python and platform

- Python **3.7 or newer** is suitable for the reference implementation.
- The optimized path relies on Unix-oriented facilities such as `os.pread` and `os.fork`; it is intended for Linux/Unix environments.
- The parallel worker path additionally uses `os.waitstatus_to_exitcode` (available in Python 3.9+). On environments where the optimized path is unavailable, the top-level driver attempts its reference fallback.
- No third-party packages are required.

For reliable use of the optimized multi-process path, use a supported Linux/Unix environment and Python 3.9+.

### Dependencies

The module uses Python standard-library modules:

| Module | Purpose |
| --- | --- |
| `gc` | Temporarily disables garbage collection during processing. |
| `marshal` | Serializes worker results between processes. |
| `os` | File access, process management, environment variables, and directory creation. |
| `sys` | Command-line arguments and exit handling. |
| `array` | Compact storage of unsigned integer latency values. |
| `bisect` | Binary searches for hour boundaries and percentile ranks. |
| `itertools` | Selects endpoint values corresponding to error responses. |
| `datetime` | Parses timestamps in the reference implementation. |
| `collections.Counter` | Histogram-based percentile calculation when useful. |

### Run as a script

From the repository root:

```bash
python level1/data_processor.py --data /path/to/data --out /path/to/output
```

The input directory must contain `requests.log`. The output directory is created if necessary, and the result is written to `summary.txt`.

If arguments are omitted, the defaults are:

- `--data /data`
- `--out /out`

Both forms are accepted:

```bash
python level1/data_processor.py --data /data --out /out
python level1/data_processor.py --data=/data --out=/out
```

### Import and call from Python

```python
from level1.data_processor import main

main(data_dir="/path/to/data", out_dir="/path/to/output")
```

If `level1` is not an importable package in your checkout (for example, it has no `__init__.py` and your environment does not use namespace packages), add the `level1` directory to `sys.path` or run/import the module by its file path.

## 3. API Reference

There is no `DataProcessor` class in the current source. The supported top-level entry points are the following functions.

### `main(data_dir, out_dir)`

```python
main(data_dir, out_dir) -> None
```

Runs the processing pipeline.

| Parameter | Type | Description |
| --- | --- | --- |
| `data_dir` | `str` or path-like value accepted by `os.path.join` | Directory containing `requests.log`. |
| `out_dir` | `str` or path-like value accepted by `os.path.join` | Destination directory for `summary.txt`. |

**Return:** `None`. Writes the summary file as a side effect.

**Behavior and errors:** Disables garbage collection, tries the optimized pipeline, retries the optimized pipeline in single-process mode after an exception, and finally tries the reference implementation. The reference implementation can raise normal file and parsing exceptions, including `FileNotFoundError`, `PermissionError`, `ValueError`, and `OSError`. The driver does not provide a structured error result.

### `fast_main(data_dir, out_dir, single=False)`

```python
fast_main(data_dir, out_dir, single=False) -> None
```

Runs the optimized byte-oriented implementation.

| Parameter | Type | Description |
| --- | --- | --- |
| `data_dir` | `str` or path-like | Directory containing `requests.log`. |
| `out_dir` | `str` or path-like | Destination directory for `summary.txt`. |
| `single` | `bool` | If true, disables multi-process execution. Defaults to false. |

**Return:** `None`.

**Potential exceptions:** `Fallback` when the data or file state does not satisfy fast-path assumptions; `WorkerFailed` when a worker process fails; and operating-system, conversion, or resource exceptions such as `OSError`, `ValueError`, and `OverflowError`. The public `main` driver handles these by retrying/falling back.

### `reference_main(data_dir, out_dir)`

```python
reference_main(data_dir, out_dir) -> None
```

Runs the simpler Python reference implementation. It parses each timestamp with `datetime.strptime`, groups latency values by hour and endpoint, sorts each group, and computes the requested percentiles.

| Parameter | Type | Description |
| --- | --- | --- |
| `data_dir` | `str` or path-like | Directory containing `requests.log`. |
| `out_dir` | `str` or path-like | Destination directory for `summary.txt`. |

**Return:** `None`.

**Potential exceptions:** `FileNotFoundError`, `PermissionError`, `OSError` for file and directory operations; `ValueError` for malformed rows, invalid integer fields, or timestamps that do not match the expected format; and `UnicodeDecodeError` for invalid UTF-8 input.

### Input and output formats

#### Input: `requests.log`

The file is expected to contain one request per line, with five single-space-separated fields:

```text
timestamp method endpoint status latency_us
```

Example:

```text
2025-01-15T10:02:03.123456Z GET /api/items 200 1250
2025-01-15T10:02:05.654321Z POST /api/items 503 8420
2025-01-15T11:00:01.000000Z GET /health 200 310
```

Fields:

| Field | Meaning |
| --- | --- |
| `timestamp` | Request timestamp. The reference parser expects `%Y-%m-%dT%H:%M:%S.%f%z`. |
| `method` | HTTP method; retained in the input format but not used in the aggregation. |
| `endpoint` | Endpoint/path used as the grouping key. |
| `status` | Integer HTTP status code. Codes from 500 through 599 count as errors. |
| `latency_us` | Non-negative integer latency in microseconds; the optimized path stores values as unsigned integers. |

The fast parser is optimized for conventional space-separated records and UTC timestamps ending in `Z`. Inputs outside its assumptions trigger the reference fallback in the top-level driver. The reference parser uses `split(" ")`, so each row must resolve to exactly five fields.

#### Output: `summary.txt`

Each output line has eight space-separated columns:

```text
hour_utc endpoint request_count p50_us p95_us p99_us error_count error_rate_ppm
```

Example:

```text
2025-01-15T10:00:00Z /api/items 2 1250 8420 8420 1 500000
2025-01-15T11:00:00Z /health 1 310 310 310 0 0
```

Groups are sorted by hour and endpoint. Percentiles use the nearest-rank convention: rank is `ceil(p * n / 100)`, using one-based indexing. Error rate is `error_count * 1_000_000 // request_count`.

## 4. Code Examples

### Process a log directory

```python
from level1.data_processor import main

# Reads ./sample_data/requests.log and writes ./output/summary.txt.
main("./sample_data", "./output")
```

### Run from the command line

```bash
python level1/data_processor.py --data ./sample_data --out ./output
cat ./output/summary.txt
```

### Use the reference implementation directly

```python
from level1.data_processor import reference_main

reference_main("./sample_data", "./output_reference")
```

### Create a minimal sample input

```bash
mkdir -p sample_data output
cat > sample_data/requests.log <<'EOF'
2025-01-15T10:02:03.123456Z GET /api/items 200 1250
2025-01-15T10:02:05.654321Z POST /api/items 503 8420
2025-01-15T11:00:01.000000Z GET /health 200 310
EOF

python level1/data_processor.py --data sample_data --out output
cat output/summary.txt
```

## 5. Error Handling & Edge Cases

### Empty input

An empty `requests.log` is valid. The pipeline produces an empty `summary.txt` file. The output directory is created if it does not already exist.

### Invalid or unexpected records

- The fast parser checks the row token count and selected timestamp structure. If its assumptions are not met, it raises the internal `Fallback` signal.
- The top-level `main` function responds to `Fallback` by running the reference implementation.
- The reference implementation expects exactly five fields separated by single spaces, a parseable timestamp with a fractional second and timezone, and integer status and latency values.
- Malformed rows, invalid timestamps, or non-integer numeric fields can raise `ValueError` in the reference implementation.
- Latencies are stored in an unsigned integer array on the fast path. Negative or out-of-range values may cause the fast path to fall back; the reference path converts latency with `int()` and does not impose the same unsigned-array constraint.
- A malformed status value may cause integer conversion to fail.

The module does not expose a configurable validation policy or collect per-row validation errors; malformed input can cause processing to fail.

### File I/O and worker failures

- A missing input file raises `FileNotFoundError`.
- Insufficient permissions or other filesystem problems can raise `PermissionError` or `OSError`.
- If the file changes while the optimized parser is reading it, the fast path can request a fallback.
- If a worker process fails for a reason unrelated to the input (for example, resource exhaustion), the driver retries without parallel workers and then attempts the reference implementation.
- Output writing can fail if the destination cannot be created or written.

### Empty groups and statistics

The pipeline only creates groups for records it reads. It does not emit rows for hours/endpoints with no requests. Since every emitted group has at least one record, percentile and PPM calculations have a non-zero request count.

### Tuning the optimized path

Two environment variables affect the optimized implementation:

| Variable | Default | Description |
| --- | --- | --- |
| `A_INGEST_CHUNK` | `262144` | Chunk size in bytes used while ingesting the file. |
| `A_INGEST_WORKERS` | Detected CPU budget | Overrides the detected worker count; values are clamped to at least one. |

Example:

```bash
A_INGEST_CHUNK=524288 A_INGEST_WORKERS=4 \
  python level1/data_processor.py --data ./sample_data --out ./output
```

The worker count detection also considers CPU affinity and, where available, cgroup CPU quotas.

---

## Scope note

The numerical-dataset API mentioned in the project task (constructor, `add_data`, summary statistics, filtering, transformations, and CSV persistence) is not implemented in the current `level1/data_processor.py`. If that API is the intended assignment, the source file needs to be supplied or corrected before that API can be documented accurately.
