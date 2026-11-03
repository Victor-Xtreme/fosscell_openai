# Request Log Data Processor

> **Implementation note:** The supplied `data_processor.py` does not define a `DataProcessor` class. It does not implement the `add_data`, statistical-summary, filtering, transformation, or CSV methods described in the original task brief. Instead, it is a command-line processor for request logs. This README documents the implementation that is actually present in the supplied source file; it does not invent the missing class or methods.

## 1. Overview

`data_processor.py` reads a request log named `requests.log`, groups requests by **hour and endpoint**, and writes an aggregated report named `summary.txt`.

For each hour/endpoint group, the program calculates:

- The number of requests.
- The 50th, 95th, and 99th percentile request latencies.
- The number of requests with an HTTP status code from `500` through `599`.
- The 5xx error rate in parts per million (PPM).

The latency values are interpreted as integer microseconds (`latency_us`). The HTTP method is read from the input but is not used as part of the grouping key or output.

The implementation is designed for large log files. Its fast path reads the file in chunks, divides work across processes where appropriate, and uses optimized routines for counting errors and calculating percentile ranks. If the input does not match assumptions made by the fast parser, the program falls back to a simpler reference implementation. If the parallel fast path fails, it retries the fast path in single-process mode before using the reference implementation.

This program is a **request-log aggregation tool**. It does not perform general-purpose numerical dataset ingestion, arbitrary statistical analysis, data filtering or linear transformations, and it does not read or write CSV files.

## 2. Input and Output Format

### Input: `requests.log`

The program expects the input file to be located inside the data directory supplied on the command line. Its filename is fixed as `requests.log`.

Each record must contain five fields in this order:

| Position | Field | Description | Example |
| --- | --- | --- | --- |
| 1 | `timestamp` | Timestamp with a fractional-second component and timezone | `2026-11-03T10:00:00.100Z` |
| 2 | `method` | HTTP method | `GET` |
| 3 | `endpoint` | Request path or endpoint | `/api/items` |
| 4 | `status` | HTTP status code | `200` |
| 5 | `latency_us` | Integer latency in microseconds | `1200` |

Use one record per line and separate fields with spaces. For the optimized path, timestamps are expected to use the UTC `Z` form shown above and to contain fractional seconds. Keep endpoint names free of whitespace so each record remains five fields.

Example input:

```text
2026-11-03T10:00:00.100Z GET /api/items 200 1200
2026-11-03T10:00:00.200Z POST /api/items 500 3000
2026-11-03T10:00:00.300Z GET /api/items 200 2400
2026-11-03T10:01:00.100Z GET /health 200 400
```

### Output: `summary.txt`

The program creates the output directory if it does not already exist and writes `summary.txt` inside it. Existing output with that name is overwritten.

Each output line contains eight whitespace-separated fields:

```text
hour endpoint request_count p50_us p95_us p99_us error_count error_ppm
```

For the example input above, the output is:

```text
2026-11-03T10:00:00Z /api/items 3 2400 3000 3000 1 333333
2026-11-03T10:01:00Z /health 1 400 400 400 0 0
```

The percentile calculation uses the nearest-rank method. For percentile `p` and group size `n`, the one-based rank is `ceil(p * n / 100)`. The reported value is the value at that rank after sorting the group's latencies. The error rate is calculated using integer division:

```text
error_ppm = error_count * 1,000,000 // request_count
```

Groups are written in sorted hour/endpoint order.

## 3. Requirements & Setup

### Requirements

- **Python:** Python 3.9 or later. The source calls `os.waitstatus_to_exitcode`, which was added in Python 3.9.
- **Platform:** A Unix-like operating system that provides `os.fork` and `os.pread` (for example, Linux). The implementation uses process forking and POSIX-style file operations, so it is not suitable for native Windows execution as written.
- **Dependencies:** Python standard library only. There are no third-party packages to install.

The source imports or uses these standard-library modules:

| Module | Purpose |
| --- | --- |
| `gc` | Temporarily disables garbage collection while the main processing routine runs. |
| `marshal` | Serializes worker-process results for transfer through pipes. |
| `os` | File access, process creation, pipes, CPU detection, and directory operations. |
| `sys` | Command-line argument handling and process exit. |
| `array` | Compact storage for unsigned integer latency values. |
| `bisect` | Efficient rank and timestamp boundary searches. |
| `itertools` | Selects endpoints associated with 5xx responses. |
| `collections.Counter` | Used by the histogram-based percentile optimization. |
| `datetime` | Used by the reference implementation to parse timestamps. |

The original task brief mentioned `math`, `csv`, and `typing`; those modules are **not** used by the supplied source file.

### Run from the command line

Place the script and your input directory wherever convenient. The expected layout can be as simple as:

```text
project/
├── data_processor.py
├── data/
│   └── requests.log
└── out/
```

Run the processor from the directory containing `data_processor.py`:

```bash
python3 data_processor.py --data ./data --out ./out
```

After a successful run, the report will be available at:

```text
./out/summary.txt
```

Both command-line arguments are optional. The defaults are `/data` for the input directory and `/out` for the output directory:

```bash
python3 data_processor.py
```

The script also accepts equals-style arguments:

```bash
python3 data_processor.py --data=./data --out=./out
```

To display the command-line usage message:

```bash
python3 data_processor.py --help
```

### Import and call from Python

The module can also be imported and its top-level driver called directly:

```python
from data_processor import main

main(data_dir="./data", out_dir="./out")
```

`main` writes the report to disk and returns `None`. It does not return the aggregated results as a Python object.

## 4. API Reference

There is no `DataProcessor` class in the supplied source. The main callable entry points are module-level functions.

### `main(data_dir, out_dir)`

```python
main(data_dir, out_dir)
```

| Parameter | Type | Description |
| --- | --- | --- |
| `data_dir` | `str` or path-like | Directory containing `requests.log`. |
| `out_dir` | `str` or path-like | Directory in which `summary.txt` will be created. |

**Returns:** `None`. The result is persisted to `summary.txt`.

**Behavior:** Disables garbage collection for the processing run, tries `fast_main` with its normal worker configuration, and uses `reference_main` if the fast parser raises the internal `Fallback` signal. If another exception occurs during the fast run, it retries `fast_main` with `single=True`; if that retry also fails, it invokes `reference_main`.

**Exceptions:** Exceptions raised by the final `reference_main` attempt are not swallowed. These can include:

- `FileNotFoundError` or another `OSError` for input/output problems.
- `ValueError` for malformed records, timestamps, or integer fields.
- `UnicodeDecodeError` if the reference implementation cannot decode the input as UTF-8.

### `fast_main(data_dir, out_dir, single=False)`

```python
fast_main(data_dir, out_dir, single=False)
```

| Parameter | Type | Description |
| --- | --- | --- |
| `data_dir` | `str` or path-like | Directory containing `requests.log`. |
| `out_dir` | `str` or path-like | Destination directory for `summary.txt`. |
| `single` | `bool` | When `True`, disables the program's multi-process work distribution and runs the fast processing path in single-process mode. Defaults to `False`. |

**Returns:** `None`. Writes `summary.txt` to `out_dir`.

**Exceptions:**

- The internal `Fallback` exception indicates that the input or a value does not fit assumptions of the optimized parser. `main` catches this and invokes the reference implementation.
- The internal `WorkerFailed` exception indicates that a worker process failed unexpectedly. `main` catches general exceptions from the fast path and retries in single-process mode.
- File-system errors such as `FileNotFoundError`, `PermissionError`, and other `OSError` subclasses may occur.
- Other parsing or processing errors may propagate when this function is called directly.

`Fallback` and `WorkerFailed` are implementation control-flow exceptions, not part of a stable application-level result API.

### `reference_main(data_dir, out_dir)`

```python
reference_main(data_dir, out_dir)
```

| Parameter | Type | Description |
| --- | --- | --- |
| `data_dir` | `str` or path-like | Directory containing `requests.log`. |
| `out_dir` | `str` or path-like | Destination directory for `summary.txt`. |

**Returns:** `None`. Reads the input using the reference parsing path, calculates the same group statistics, and writes `summary.txt`.

**Exceptions:**

- `FileNotFoundError` and other `OSError` subclasses for file-system failures.
- `ValueError` if a line does not split into the expected fields, a timestamp cannot be parsed, or a numeric field cannot be converted to an integer.
- `UnicodeDecodeError` if the file is not valid UTF-8.

The reference implementation uses Python's `datetime.strptime` with the format `%Y-%m-%dT%H:%M:%S.%f%z`.

### Command-line argument parser

The private helper `_parse_args(argv)` reads `--data` and `--out` options. It is an implementation detail rather than a supported public API. Unrecognized arguments or missing option values terminate the process with an error message; `-h` and `--help` print usage and exit successfully.

## 5. Code Examples

### Example 1: Create a sample log and run the script

Run these commands from the directory containing `data_processor.py`:

```bash
mkdir -p ./data ./out

cat > ./data/requests.log <<'LOG_EOF'
2026-11-03T10:00:00.100Z GET /api/items 200 1200
2026-11-03T10:00:00.200Z POST /api/items 500 3000
2026-11-03T10:00:00.300Z GET /api/items 200 2400
2026-11-03T10:01:00.100Z GET /health 200 400
LOG_EOF

python3 data_processor.py --data ./data --out ./out
cat ./out/summary.txt
```

Expected report:

```text
2026-11-03T10:00:00Z /api/items 3 2400 3000 3000 1 333333
2026-11-03T10:01:00Z /health 1 400 400 400 0 0
```

### Example 2: Run with the default directories

The default input and output directories are `/data` and `/out`. Ensure those directories exist and are readable/writable by the account running the script:

```bash
mkdir -p /data /out
cp ./data/requests.log /data/requests.log
python3 data_processor.py
cat /out/summary.txt
```

### Example 3: Call the module from another Python file

Create a file such as `run_processor.py` beside `data_processor.py`:

```python
from data_processor import main

if __name__ == "__main__":
    main("./data", "./out")
    print("Report written to ./out/summary.txt")
```

Run it with:

```bash
python3 run_processor.py
```

## 6. Error Handling & Edge Cases

### Empty input file

An empty `requests.log` is accepted. The program writes an empty `summary.txt` because there are no hour/endpoint groups to report.

### Unsupported or unexpected input

The optimized parser checks assumptions about the record layout and timestamp shape. When it detects input outside the fast path's assumptions, it raises the internal `Fallback` signal and `main` switches to `reference_main`.

The reference implementation is not a general-purpose forgiving parser. It expects five fields separated by single spaces and a timestamp parseable with `%Y-%m-%dT%H:%M:%S.%f%z`. Malformed records can therefore raise `ValueError` rather than being skipped.

### Invalid status and latency values

Status values must be convertible to integers. A status is counted as an error only when its numeric value is between `500` and `599`, inclusive.

Latency values must be convertible to integers. The fast path stores them in an unsigned integer array, so negative values and values too large for that representation trigger fallback. The reference path uses Python integers and may accept values that the fast path cannot represent, provided they can be parsed as integers.

### Timestamp assumptions

The optimized parser expects a timestamp with the general shape `YYYY-MM-DDTHH:MM:SS.fractionZ`. The reference parser uses `%Y-%m-%dT%H:%M:%S.%f%z`. Use UTC timestamps ending in `Z` with fractional seconds for the documented input format and best compatibility with the fast path.

### File-system errors

The input file must exist and be readable. The output directory is created automatically when needed, but the process must have permission to create the directory and write `summary.txt`. File-system errors are not silently converted into successful output; an error from the final reference attempt can propagate to the caller or command line.

### Parallel worker failures

The fast path may create worker processes. If that path fails, `main` first retries with `single=True`, then falls back to the reference implementation if the retry also fails. Calling `fast_main` directly bypasses this recovery orchestration.

### Duplicate records and grouping

Every valid input record contributes to its hour/endpoint group's request count and latency distribution. The HTTP method is not included in the grouping key, so requests to the same endpoint in the same hour are combined even if their methods differ.

### Output behavior

The output file is overwritten on each successful run. The program does not append to previous reports, retain a database, or export CSV.

## 7. Performance Configuration

Two environment variables can tune the fast path:

| Variable | Default | Purpose |
| --- | --- | --- |
| `A_INGEST_CHUNK` | `262144` bytes (256 KiB) | Parser chunk size. It is read when the module is imported. |
| `A_INGEST_WORKERS` | Detected CPU budget | Overrides the number of worker processes used by the fast path. The value is converted to an integer and clamped to at least one worker. |

For example:

```bash
A_INGEST_WORKERS=4 python3 data_processor.py --data ./data --out ./out
```

The code also caps the number of queued tasks internally. Leave the defaults unless you have measured a reason to tune them. `A_INGEST_CHUNK` should be set to a positive integer.

## 8. Scope and Limitations

- No `DataProcessor` class is present in the supplied file.
- There are no `add_data`, `batch_add`, mean, median, variance, standard-deviation, summary-statistics, filtering, or linear-transformation methods.
- The program does not import or use `csv`; its report is a space-separated text file.
- The program writes its result to disk and does not expose the computed groups through a return value.
- The script relies on Unix process and file APIs and is not portable to native Windows without code changes.
- The optimized path is designed around the documented request-log format; malformed input may be rejected by the reference parser as well.

If the intended assignment is specifically to document a numerical `DataProcessor` class with CSV persistence, the matching implementation file is still needed. The README can then be revised to document that class and its actual behavior.
