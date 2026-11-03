# Log Ingestion and Latency Summary

> **Source note:** The supplied source code is an ingestion script (`data_processor.py` implementation). It does not define a `DataProcessor` class and does not implement general-purpose numerical dataset transformations or CSV persistence. This README documents the behavior that is actually present in the supplied code.

## 1. Overview

This program reads a request log named `requests.log`, groups requests by **UTC hour and endpoint**, and writes a summary to `summary.txt`.

For each hour/endpoint group, it calculates:

- Request count
- 50th, 95th, and 99th percentile latency, in microseconds
- Number of server-error responses (HTTP status codes 500–599)
- Server-error rate in parts per million (PPM)

The implementation is designed for large log files. Its optimized path reads the file in chunks, partitions work across processes on supported POSIX systems, stores latency values in compact unsigned-integer arrays, and calculates percentile ranks without always sorting an entire group. If the input does not meet assumptions required by the optimized parser, the program retries using a simpler reference implementation.

The program writes a space-separated text report. **It does not write CSV files**, and the supplied code does not provide a general-purpose `DataProcessor` API.

## 2. Requirements and Installation

### Requirements

- Python 3.8 or newer is recommended.
- A POSIX operating system is required for the multiprocessing fast path because it uses `os.fork()` and `os.pread()`.
- The optimized path uses only Python's standard library; no third-party packages are imported as runtime requirements.
- On platforms without the required POSIX facilities, the optimized path may fail and the program may fall back to the reference implementation. The reference implementation itself uses standard Python file and date/time handling.

### Installation

Keep the supplied Python source as `data_processor.py`. No package installation is required.

```bash
python --version
python data_processor.py --help
```

## 3. Input File Format

By default, the program looks for:

```text
/data/requests.log
```

Each record must contain five fields, in this order:

```text
timestamp method endpoint status latency_us
```

Example:

```text
2026-10-01T12:00:00.123456Z GET /api/items 200 1450
2026-10-01T12:00:01.654321Z POST /api/items 503 8200
2026-10-01T12:01:03.000001Z GET /health 200 320
```

Field descriptions:

| Field | Description |
|---|---|
| `timestamp` | Request timestamp. The fast path expects a timestamp beginning `YYYY-MM-DDTHH`, containing the expected separators, and ending in `Z`. The reference parser uses `%Y-%m-%dT%H:%M:%S.%f%z`. |
| `method` | HTTP method, such as `GET` or `POST`. It is parsed but not used in the aggregation. |
| `endpoint` | Request endpoint/path used as a grouping key. |
| `status` | HTTP status code. Codes from 500 through 599 count as errors. |
| `latency_us` | Latency value in microseconds. The fast path stores values as unsigned 32-bit integers. |

The optimized parser expects five whitespace-separated tokens per line. The reference parser uses a literal single-space split, so malformed spacing or malformed records can still cause the reference path to raise an error.

### Input assumptions and fallback behavior

The optimized path is specialized for the expected log format. It falls back to the reference algorithm when it detects conditions such as an unexpected token count, an unsuitable timestamp shape, a latency value that cannot be stored as an unsigned 32-bit integer, or a file that changes while being read.

For consistent hourly grouping and the `Z` output suffix, provide timestamps in UTC using the `Z` suffix. The fast path groups by the first 13 timestamp bytes (`YYYY-MM-DDTHH`).

## 4. Running the Program

### Basic usage

```bash
python data_processor.py
```

The default directories are:

- Input directory: `/data`
- Output directory: `/out`

The program reads `/data/requests.log` and writes `/out/summary.txt`.

### Specify input and output directories

```bash
python data_processor.py --data /path/to/input --out /path/to/output
```

The program expects the input file to be named `requests.log` inside the directory supplied to `--data`. It creates the output directory if it does not already exist.

Both separated and equals-style arguments are supported:

```bash
python data_processor.py --data=/path/to/input --out=/path/to/output
```

### Help

```bash
python data_processor.py --help
```

## 5. Configuration

The program supports two environment variables:

| Variable | Default | Purpose |
|---|---:|---|
| `A_INGEST_CHUNK` | `262144` bytes (256 KiB) | Controls the nominal byte chunk size used while reading and parsing the input file. |
| `A_INGEST_WORKERS` | Automatically detected | Overrides the detected CPU worker count. The value is converted to an integer and limited to at least one worker. |

Examples:

```bash
A_INGEST_CHUNK=524288 python data_processor.py
A_INGEST_WORKERS=4 python data_processor.py --data /data --out /out
```

When no worker override is provided, the program checks the available process CPU count or CPU affinity and attempts to account for Linux cgroup CPU quotas. Small input files are processed by one worker. The program also limits the number of queued tasks.

## 6. Output Format

The output file is:

```text
summary.txt
```

It contains one line for each hour/endpoint group, ordered by the group key (hour, then endpoint).

Each line has eight space-separated fields:

```text
hour endpoint request_count p50 p95 p99 error_count error_ppm
```

Example:

```text
2026-10-01T12:00:00Z /api/items 2 1450 8200 8200 1 500000
2026-10-01T12:01:00Z /health 1 320 320 320 0 0
```

| Output field | Meaning |
|---|---|
| `hour` | Hour bucket, formatted as `YYYY-MM-DDTHH:00:00Z`. |
| `endpoint` | Endpoint associated with the group. |
| `request_count` | Number of requests in the group. |
| `p50` | 50th percentile latency in microseconds. |
| `p95` | 95th percentile latency in microseconds. |
| `p99` | 99th percentile latency in microseconds. |
| `error_count` | Number of responses with status 500–599. |
| `error_ppm` | Error rate in parts per million: `error_count * 1,000,000 // request_count`. |

### Percentile definition

The program uses the nearest-rank percentile convention. For percentile `p` and a sorted group of `n` latency values, the one-based rank is:

```text
rank = ceil(p * n / 100)
```

The reported percentile is the value at that rank (using zero-based index `rank - 1`). Integer arithmetic in the implementation is equivalent to:

```python
rank = (p * n + 99) // 100
```

The PPM error rate is also calculated with integer floor division.

## 7. Processing Design

The implementation has an optimized path and a reference path.

### Optimized path

1. Opens `requests.log` and obtains its size.
2. Splits the file into ranges aligned to line boundaries.
3. Reads complete-line chunks using positional reads.
4. Parses the five fields and groups latency values by hour and endpoint.
5. Counts 5xx responses.
6. Uses worker processes to process file ranges when the file and environment make parallel processing worthwhile.
7. Calculates p50, p95, and p99 for each group. It can use a histogram/counting approach for groups with relatively few distinct values; otherwise it sorts values in bounded pieces and finds the required ranks.
8. Writes the sorted summary rows to `summary.txt`.

The implementation uses compact `array('I')` buffers for latency values while accumulating data. The maximum number of elements sorted at once in a piece is controlled by the internal `PIECE` constant (1,048,576 elements).

### Reference path

The reference implementation reads the file as UTF-8 text, parses each timestamp with `datetime.strptime`, collects latency values in Python lists, sorts each group, and calculates the same percentile ranks and error counts.

### Retry and fallback behavior

The top-level `main()` function:

- Tries the optimized implementation first.
- Uses the reference implementation when the optimized parser raises its `Fallback` signal for input outside its assumptions.
- If another exception occurs during the optimized run (for example, a worker/process failure), retries the optimized implementation in single-process mode.
- If that retry also fails, runs the reference implementation.

This retry strategy is intended to preserve a simpler correctness path when the optimized execution cannot be used.

## 8. Error Handling and Operational Notes

- The output directory is created automatically.
- The input file must exist at `<data directory>/requests.log`.
- An empty input file produces an empty `summary.txt`.
- Unknown command-line arguments cause the program to exit with an error.
- `--data` and `--out` require values.
- The fast path is sensitive to the expected input layout and timestamp representation; test representative production data before deployment.
- The reference implementation is not a universal validator. Invalid records can cause parsing exceptions there as well.
- The code disables Python's cyclic garbage collector for the duration of `main()` and does not explicitly re-enable it before process exit.

## 9. Performance Considerations

- Increase or decrease `A_INGEST_CHUNK` to experiment with read/parse chunk sizes for the storage and workload.
- Set `A_INGEST_WORKERS` to control process parallelism. More workers are not always faster; storage throughput and memory availability can become bottlenecks.
- The program uses CPU quota detection where available, which is useful in containerized environments.
- The optimized path uses multiple processes for file parsing and, for sufficiently large aggregated data, percentile calculation.
- Large groups can require substantial memory because their latency values must be retained until percentile calculation is complete.
- The reference path is simpler but uses Python objects and full per-group sorting, which may require more memory and CPU time for large logs.

## 10. Limitations and Scope

Based on the supplied source code, this program:

- Processes request logs with five fields; it is not a general numerical dataset ingestion framework.
- Groups only by hour and endpoint.
- Computes only request count, p50/p95/p99 latency, 5xx count, and 5xx PPM.
- Does not expose a `DataProcessor` class or public transformation/filtering API.
- Does not persist results as CSV; it writes a space-separated `summary.txt`.
- Does not include a test suite, packaging metadata, dependency lockfile, or formal logging configuration in the supplied source.

If the intended assignment specifically requires documentation for a separate `data_processor.py` module that performs numerical dataset ingestion, statistical analysis, filtering, transformations, and CSV storage, that source file is still needed. The documentation above deliberately does not claim those features because they are not present in the supplied code.

## 11. Quick Start

```bash
# 1. Prepare the input directory and log file
mkdir -p /data /out

# 2. Put records in /data/requests.log using the five-field format

# 3. Run the processor
python data_processor.py --data /data --out /out

# 4. Inspect the summary
cat /out/summary.txt
```
