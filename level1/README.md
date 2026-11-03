# Hourly HTTP Request Log Aggregator

A high-performance, standard-library-only Python tool that processes HTTP request logs and generates hourly performance summaries for each endpoint.

For every **hour + endpoint** combination, it reports request volume, latency percentiles (p50, p95, and p99), and HTTP 5xx error counts and rates. The implementation optimizes parsing and percentile calculation for large logs, while retaining a reference implementation as a fallback.

## Features

- **Hourly endpoint-level aggregation** of HTTP request logs.
- **Latency analysis** using exact nearest-rank p50, p95, and p99 percentiles.
- **Error monitoring** with HTTP 5xx counts and error rates in parts per million (PPM).
- **Parallel processing** using multiple processes on supported POSIX systems.
- **Optimized data handling** through chunked file reads, compact unsigned-integer arrays, and specialized percentile strategies.
- **Fallback implementation** for input that does not meet the optimized parser's assumptions or when optimized processing fails.
- **No third-party Python dependencies.**

## Requirements

- Python 3.9 or newer is recommended.
- A **POSIX-compatible operating system** (such as Linux or macOS) for the optimized implementation. It uses `os.pread()` and, when parallelism is enabled, `os.fork()`.
- Sufficient RAM to retain latency samples while aggregating. Memory use grows with the size of the input.

> **Note:** This implementation is not designed to run unchanged on Windows because it relies on POSIX-specific system calls.

## Getting Started

Save the supplied script in your repository as `a_ingest_fast.py`.

### 1. Prepare the input

Place a file named `requests.log` in an input directory, such as `data/`.

Each log entry must contain **five space-separated fields**:

```text
<timestamp> <method> <endpoint> <status> <latency_us>
```

| Field | Description | Example |
| --- | --- | --- |
| `timestamp` | ISO 8601 timestamp, preferably UTC with fractional seconds and `Z` | `2026-11-03T14:05:00.000000Z` |
| `method` | HTTP method (read but not used for grouping) | `GET` |
| `endpoint` | Request path used as an aggregation key | `/api/users` |
| `status` | HTTP response status code | `503` |
| `latency_us` | Request latency in **microseconds**, as an integer | `400` |

Example `data/requests.log`:

```text
2026-11-03T14:00:00.000000Z GET /api/users 200 100
2026-11-03T14:05:00.000000Z GET /api/users 503 400
2026-11-03T14:30:00.000000Z POST /api/orders 500 1000
2026-11-03T14:45:00.000000Z GET /api/users 200 300
```

For the optimized path, use consistently formatted UTC (`Z`) timestamps and nonnegative latency values that fit in a 32-bit unsigned integer. The reference path handles some cases outside those assumptions, but input still needs to match the expected five-field format.

### 2. Run the aggregator

From your project directory:

```bash
python3 a_ingest_fast.py --data ./data --out ./out
```

The tool reads `./data/requests.log` and creates `./out/summary.txt`. The output directory is created if it does not exist.

If no arguments are supplied, the script uses `/data` as the input directory and `/out` as the output directory:

```bash
python3 a_ingest_fast.py
```

Show the available command-line options:

```bash
python3 a_ingest_fast.py --help
```

### 3. View the output

For the example input above, `out/summary.txt` contains:

```text
2026-11-03T14:00:00Z /api/orders 1 1000 1000 1000 1 1000000
2026-11-03T14:00:00Z /api/users 3 300 400 400 1 333333
```

Each output line has **eight space-separated fields**:

```text
<hour> <endpoint> <count> <p50> <p95> <p99> <errors_5xx> <error_ppm>
```

| Field | Meaning |
| --- | --- |
| `hour` | Hour bucket, shown as `YYYY-MM-DDTHH:00:00Z` |
| `endpoint` | Endpoint path |
| `count` | Number of requests in the hour for that endpoint |
| `p50` | 50th-percentile latency in microseconds |
| `p95` | 95th-percentile latency in microseconds |
| `p99` | 99th-percentile latency in microseconds |
| `errors_5xx` | Number of responses with status codes from 500 through 599 |
| `error_ppm` | 5xx errors per million requests, truncated to an integer |

Output rows are sorted by hour and then endpoint. HTTP methods are **not** used to separate groups.

## Calculations

### Latency percentiles

Percentiles use the **nearest-rank method**. For `n` sorted latency values, percentile `p` is selected at the 1-based rank:

```text
rank = ceil(p * n / 100)
```

The script calculates this for `p = 50`, `95`, and `99`. These are actual observed latency values, not interpolated estimates.

### Error rate

Only HTTP response status codes **500–599** count as errors.

```text
error_ppm = (errors_5xx * 1_000_000) // count
```

The `//` operator means the result is rounded down to an integer. For example, one 5xx error among three requests produces `333333` PPM.

## Performance Approach

Processing happens in two main phases:

1. **Parse and aggregate:** Read input in chunks, split work at line boundaries, group requests by hour and endpoint, store latency values in compact arrays, and count 5xx responses.
2. **Compute percentiles:** Calculate exact nearest-rank values for each group, using a direct sort, histogram-based counting for suitable low-cardinality data, or sorted runs with binary searches for large groups.

When supported and beneficial, multiple worker processes perform parsing and percentile computation in parallel. The implementation also includes shortcuts for time-ordered logs and chunks containing a single hour.

If the fast path cannot handle the input, the program attempts a simpler reference implementation. If a multiprocessing-related exception occurs, it first retries the optimized implementation in single-process mode.

### Configuration

Two optional environment variables control aspects of processing:

| Variable | Default | Purpose |
| --- | --- | --- |
| `A_INGEST_CHUNK` | `262144` bytes (256 KiB) | Target input chunk size |
| `A_INGEST_WORKERS` | Automatically determined | Maximum worker count (minimum of 1) |

Example on Linux or macOS:

```bash
A_INGEST_WORKERS=4 A_INGEST_CHUNK=524288 \
  python3 a_ingest_fast.py --data ./data --out ./out
```

Larger worker counts or chunks do **not** guarantee better performance; results depend on input size, endpoint distribution, CPU availability, and memory capacity. No specific benchmark or speedup is claimed here.

## Important Notes

- **Use UTC timestamps:** The optimized path groups by the timestamp's hour prefix, and output hour labels end with `Z`. It does not perform timezone conversion. UTC `Z` input avoids misleading labels.
- **Input format matters:** The optimized parser assumes five whitespace-separated fields per line. The reference implementation requires five fields separated by single spaces and a timestamp parseable by its configured format.
- **Latencies are in microseconds:** Output percentiles retain the same unit; they are not automatically converted to milliseconds.
- **Memory usage is not constant:** Latency samples are retained for percentile computation. Very large log files may require substantial RAM.
- **No output rows for empty input:** An empty input file results in an empty `summary.txt`.

## Suggested Repository Layout

```text
.
├── a_ingest_fast.py
├── README.md
├── data/
│   └── requests.log
└── out/
    └── summary.txt
```

`data/` and `out/` are illustrative local directories; they are not required to have these names. You can supply other directory paths with `--data` and `--out`.
