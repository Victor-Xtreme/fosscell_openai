# Level 1: Request Log Data Processor

## 1. Overview

`data_processor.py` is a command-line tool for analyzing web request logs. It reads `requests.log`, groups requests by hour and endpoint, calculates the request count and latency percentiles, counts server errors (HTTP status codes 500–599), and writes a summary to `summary.txt`.

The script is optimized to process large log files in chunks and, on supported systems, use multiple processes. If the input does not match the optimized parser's assumptions or the fast path fails, it retries with a simpler reference implementation.

**Note:** The checked-in `data_processor.py` does not define a `DataProcessor` class. It also does not implement numerical dataset filtering, transformations, or CSV import/export. Its actual interface is the command-line program and the module-level functions documented below. This README documents the code that is present rather than claiming APIs that are not in the file.

## 2. Requirements & Setup

- Python 3.7 or newer.
- Standard library only; no third-party packages are required. The script imports `gc`, `marshal`, `os`, `sys`, `array`, `bisect`, and `itertools`. The reference parser also imports `datetime`; the optimized low-cardinality percentile path imports `collections.Counter`.
- A POSIX-like operating system is required for the optimized implementation because it uses `os.fork()` and `os.pread()`. The source does not provide a Windows compatibility layer.

Place an input file named `requests.log` in a data directory. Run the script from the repository root, providing the input and output directories:

```bash
python level1/data_processor.py --data ./data --out ./out
```

The defaults are `/data` for input and `/out` for output. The output directory is created if needed. The result is written to `<out-directory>/summary.txt`.

### Input format

Each line must contain five space-separated fields, in this order:

```text
timestamp method endpoint status latency_us
```

For example:

```text
2025-01-15T10:12:30.123456Z GET /api/items 200 14500
2025-01-15T10:14:02.000000Z POST /api/items 503 82700
```

The timestamp includes a fractional second and a UTC offset (`Z` is accepted by the reference parser). Latency is an integer number of microseconds. The method is parsed but not included in the grouping. Rows are grouped by the timestamp's hour and endpoint.

### Output format

Each output line contains these space-separated fields:

```text
hour endpoint request_count p50_us p95_us p99_us error_count error_rate_ppm
```

Percentiles use the nearest-rank rule: rank `ceil(percentile × count)`. Error rate is integer parts per million: `error_count * 1_000_000 // request_count`. Rows are sorted by hour and endpoint.

## 3. API Reference

There is no `DataProcessor` class or CSV API in this source file. The module's callable entry points are:

### `main(data_dir, out_dir)`

Runs the processor using the fast implementation, with fallback and retry behavior.

- `data_dir` (`str`): Directory containing `requests.log`.
- `out_dir` (`str`): Directory where `summary.txt` is written (created if necessary).
- Returns: `None`.
- Errors: The implementation retries processing with fewer optimizations and then the reference implementation. If the reference implementation also fails, file-system, parsing, or conversion exceptions can propagate (for example `FileNotFoundError`, `PermissionError`, `ValueError`).

### `fast_main(data_dir, out_dir, single=False)`

Runs the chunked optimized parser and percentile calculation.

- `data_dir` (`str`): Directory containing `requests.log`.
- `out_dir` (`str`): Destination directory for `summary.txt`.
- `single` (`bool`, default `False`): Avoids multiprocessing; used internally for retrying.
- Returns: `None`.
- Errors: Raises internal `Fallback` when input is outside the fast parser's assumptions; may raise `WorkerFailed` if a worker process fails. File I/O and conversion errors may also occur.

### `reference_main(data_dir, out_dir)`

Processes the log one line at a time using Python's `datetime` parser and ordinary lists. This is the fallback implementation.

- `data_dir` (`str`): Directory containing `requests.log`.
- `out_dir` (`str`): Destination directory for `summary.txt`.
- Returns: `None`.
- Errors: Can raise file I/O errors, `ValueError` for malformed rows, timestamps, or numeric fields, and other standard parsing exceptions.

### `_parse_args(argv)`

Parses the command-line arguments. This is an internal helper.

- `argv` (`list[str]`): Argument strings, typically `sys.argv[1:]`.
- Returns: `(data_dir, out_dir)` as a tuple of strings.
- Errors: Exits the program with a usage error for unknown options or a missing option value. `-h`/`--help` prints usage and exits successfully.

### Command-line options

- `--data PATH` or `--data=PATH`: Input directory (default `/data`).
- `--out PATH` or `--out=PATH`: Output directory (default `/out`).
- `-h`, `--help`: Print usage.

Two environment variables can tune the optimized implementation:

- `A_INGEST_CHUNK`: Chunk size in bytes (default 262144).
- `A_INGEST_WORKERS`: Number of worker processes; if unset, the script detects the available CPU budget.

## 4. Code Examples

### Create sample input and run the script

From the repository root, create `data/requests.log` with these lines:

```text
2025-01-15T10:12:30.123456Z GET /api/items 200 14500
2025-01-15T10:14:02.000000Z POST /api/items 503 82700
2025-01-15T11:03:10.500000Z GET /api/items 200 22000
```

Run the processor:

```bash
python level1/data_processor.py --data ./data --out ./out
```

Then inspect `out/summary.txt`. It will have one row per hour and endpoint, with count, p50/p95/p99 latency, 5xx count, and 5xx rate in parts per million.

### Import and call from Python

The script's `main` function can be imported and called directly. Run this from the repository root after creating `data/requests.log`:

```python
from level1.data_processor import main

main("./data", "./out")
```

The module does not expose methods for adding arbitrary numbers, calculating a general dataset summary, filtering values, transforming values, or reading/writing CSV files.

## 5. Error Handling & Edge Cases

- **Empty input file:** Produces an empty `summary.txt` file.
- **Unexpected log layout:** The fast parser signals an internal fallback when rows do not match its expected structure. `main` then tries the reference parser, which expects exactly five space-separated fields and a parseable timestamp.
- **Invalid timestamps, statuses, or latencies:** The reference parser uses `datetime.strptime` and `int()`. Invalid values raise parsing/conversion errors; if fallback also fails, the error can propagate to the caller.
- **File and directory errors:** Missing or unreadable `requests.log`, or an unwritable output directory, can raise standard operating-system exceptions such as `FileNotFoundError` or `PermissionError`.
- **Multiprocessing or resource failure:** `main` retries the fast path in single-process mode and then tries the reference parser.
- **Zero requests in a group:** Groups only exist when at least one request is present, so percentile and error-rate calculations do not divide by zero.
- **CSV operations:** None are implemented; data is read from a text log and output as a text summary.
