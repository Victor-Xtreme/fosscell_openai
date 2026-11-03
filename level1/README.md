# Level 1: Request Log Summarizer

## Overview

`data_processor.py` ingests an HTTP request log named `requests.log` and writes a compact per-hour, per-endpoint report to `summary.txt`. For each hour and endpoint, it reports request count, latency percentiles (p50, p95, p99), number of 5xx responses, and the 5xx rate in parts per million.

The implementation is optimized for large, space-delimited log files: it reads whole-line chunks, groups and aggregates records, and can use worker processes for parsing and percentile calculation. A reference parser is used when the optimized parser cannot accept the input format or parallel processing fails.

This is not a general-purpose numerical dataset API. The file does not define a `DataProcessor` class and does not provide arbitrary filtering/transformation methods or CSV persistence. Its persistent output is plain-text `summary.txt`.

## Requirements and setup

- Python 3.7 or later. The optimized multiprocessing path requires a POSIX-like platform with `os.fork`, `os.pread`, and `os.pipe`; these are not available on standard Windows Python. Run the script in a Linux/POSIX environment (or a compatible container) for its intended fast path.
- Standard library only. Modules used include `gc`, `marshal`, `os`, `sys`, `array`, `bisect`, `itertools`, and (in fallback mode) `datetime`; `collections` is imported lazily by the histogram optimization.
- No installation or third-party packages are required.

Run from the project root:

```bash
python level1/data_processor.py --data /path/to/input --out /path/to/output
```

The input directory must contain `requests.log`. The output directory is created if needed and receives `summary.txt`. If flags are omitted, the defaults are `/data` and `/out`:

```bash
python level1/data_processor.py
```

The script can also be imported:

```python
from level1 import data_processor

data_processor.main("/path/to/input", "/path/to/output")
```

## Input format

Each record is expected to be a line containing five space-separated fields:

```text
TIMESTAMP METHOD ENDPOINT STATUS LATENCY_US
```

Example:

```text
2025-03-08T12:34:56.123456+0000 GET /health 200 840
2025-03-08T12:35:04.000000+0000 POST /api/items 503 12000
```

The fast path expects timestamps in a sortable ISO-like form with the hour at the prefix `YYYY-MM-DDTHH`, seconds and a fractional portion, and a trailing `Z`. The reference path parses timestamps using `%Y-%m-%dT%H:%M:%S.%f%z`, which requires a fractional component and a UTC offset. Status is interpreted numerically; 500 through 599 count as errors. Latency must be a non-negative integer that fits the platform's unsigned C `I` array type for the fast path. Inputs outside fast-path assumptions may be processed by the reference implementation; malformed records may fail there as well.

## Output format

`summary.txt` contains one row per `(hour, endpoint)`, sorted by hour and endpoint:

```text
YYYY-MM-DDTHH:00:00Z ENDPOINT COUNT P50 P95 P99 ERRORS ERRORS_PER_MILLION
```

Percentiles use the nearest-rank convention: for percentile `p`, the selected one-based rank is `ceil(p * count / 100)`. The error rate is integer floor division: `errors * 1_000_000 // count`.

Example:

```text
2025-03-08T12:00:00Z /api/items 2 840 12000 12000 1 500000
```

## Configuration

- `--data DIR`: Directory containing `requests.log` (default `/data`).
- `--out DIR`: Destination directory for `summary.txt` (default `/out`).
- `A_INGEST_WORKERS`: Optional worker-process count. Without it, the program estimates the available CPU budget and considers cgroup CPU quota.
- `A_INGEST_CHUNK`: Optional chunk size in bytes used by the parser (default 262144).

For example:

```bash
A_INGEST_WORKERS=4 A_INGEST_CHUNK=524288 \
  python level1/data_processor.py --data ./data --out ./out
```

Small files are processed by one worker. Larger files may use multiple processes for ingestion and percentile work. Worker failures trigger a single-process retry; data that does not meet fast-path assumptions triggers the reference implementation.

## Processing behavior

1. The input file is divided into ranges aligned to line boundaries.
2. Records are parsed and grouped by hour and endpoint.
3. Latency values are accumulated and 5xx responses counted.
4. Percentile ranks are calculated using sorting, with histogram and chunked-sort strategies for suitable or large groups.
5. Results are written to `summary.txt`.

The fast path uses byte-level parsing and assumes records have five whitespace-separated fields. The fallback uses Python text parsing and `datetime.strptime`. The fallback preserves the reference behavior for supported input, but it does not make malformed data valid.

## API reference

There are no public `DataProcessor` methods in this module. The callable entry points are module-level functions:

### `main(data_dir, out_dir) -> None`

Runs the optimized processing path and writes the report. It retries in single-process mode after general fast-path failures, then falls back to the reference implementation if the retry fails.

- `data_dir` (`str` or path-like): Directory containing `requests.log`.
- `out_dir` (`str` or path-like): Directory for `summary.txt`.
- Errors: File access and output errors can propagate from the final reference attempt. The retry logic catches broad exceptions from the fast path, so underlying fast-path exceptions are not necessarily exposed directly.

### `fast_main(data_dir, out_dir, single=False) -> None`

Runs the optimized parser and aggregator.

- `data_dir`: Input directory.
- `out_dir`: Output directory.
- `single` (`bool`): If true, avoid worker processes.
- Errors: `Fallback` indicates data or file state incompatible with the fast path; `WorkerFailed` indicates a child process failure; filesystem errors may propagate.

### `reference_main(data_dir, out_dir) -> None`

Runs the text-based reference implementation, parsing timestamps with `datetime.strptime` and sorting each group of latency values.

- `data_dir`: Input directory.
- `out_dir`: Output directory.
- Errors: Standard file errors, `ValueError` for malformed timestamps or numeric fields, and unpacking errors for malformed rows can propagate.

### `parse_chunk(buf, acc) -> None`

Internal parsing helper. Parses complete request lines from a bytes object and accumulates data into `acc`. Raises `Fallback` if the chunk does not meet fast-path assumptions.

### `process_range(fd, a, b, acc) -> None`

Internal helper that reads and parses complete lines from byte offsets `[a, b)` of an open file descriptor.

The remaining underscore-prefixed functions and helpers (`_cpu_budget`, `_count_errors`, `_add_segment`, `_add_general`, `_histogram_ranks`, and others) implement internal parsing, grouping, multiprocessing, and percentile logic; they are not a stable user-facing API.

## Example

```python
from pathlib import Path
from level1.data_processor import main

input_dir = Path("data")   # contains requests.log
output_dir = Path("out")
main(input_dir, output_dir)

print((output_dir / "summary.txt").read_text(encoding="utf-8"))
```

Command-line equivalent:

```bash
python level1/data_processor.py --data ./data --out ./out
```

## Error handling and edge cases

- **Empty input file:** Produces an empty `summary.txt`.
- **Malformed rows:** Fast parsing detects many format mismatches and switches to the reference parser. The reference parser expects exactly five space-separated fields; malformed rows can raise `ValueError` or related parsing exceptions.
- **Timestamps:** Reference parsing requires a valid timestamp with fractional seconds and timezone offset. Timestamp formats outside that pattern can fail.
- **Status and latency:** Status values must be parseable as integers. Latency values must be parseable as integers; the fast path stores them in an unsigned integer array, so negative or out-of-range values cause fallback. Fallback converts latency to Python `int` but still rejects non-integer text.
- **Changing input:** If the input changes while the fast path is reading it, the fast parser raises `Fallback` and retries with the reference implementation.
- **Multiprocessing failures:** If workers fail, `main` retries the optimized route with a single process before invoking the reference path.
- **File I/O:** The input file must exist and be readable. The output directory is created as needed; permissions, invalid paths, or disk errors can still cause I/O exceptions.
- **Platform support:** The implementation uses POSIX process and file APIs. The CLI's default fallback strategy is not a portability layer for platforms without `fork`/`pread`.

## Scope notes

The task brief mentions `math`, `csv`, `typing`, a `DataProcessor` class, in-memory dataset filtering and transformations, and CSV load/export methods. None of those appear in the supplied `level1/data_processor.py`. This README documents the actual request-log summarizer instead; add those sections only if the implementation is changed to provide that API.
