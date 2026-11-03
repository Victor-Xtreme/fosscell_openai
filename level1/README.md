# Request Log Data Processor

`data_processor.py` is a command-line processor for `requests.log`. It groups request records by UTC hour and endpoint, then writes a compact latency and server-error summary to `summary.txt`. A multiprocessing fast path handles large, conventionally formatted logs; a reference implementation retries inputs the fast parser cannot safely handle.

This module does **not** define a `DataProcessor` class and does not provide general-purpose dataset ingestion, mean/median/variance calculations, filtering, or transformations. Its entry points are `main`, `fast_main`, and `reference_main`; the other helpers are implementation details.

## Requirements and setup

- Python 3.7 or later is the intended baseline. The fast path uses Unix interfaces (`os.fork`, `os.pread`, and pipes), so it requires a Unix-like platform. On platforms without these interfaces, the fast path cannot run; the fallback may work if the input is valid.
- The module uses standard-library modules: `gc`, `marshal`, `os`, `sys`, `array`, `bisect`, `itertools`, and (for fallback) `datetime`; `collections.Counter` is imported lazily for a histogram optimization.
- No third-party packages are required.

Place `data_processor.py` alongside a directory containing `requests.log`, then run:

```sh
python data_processor.py --data ./data --out ./out
```

`--data` and `--out` may also be written as `--data=./data` and `--out=./out`. Defaults are `/data` and `/out`. To import the module:

```python
import data_processor

data_processor.main("./data", "./out")
```

The output directory is created if needed. The program writes or replaces `summary.txt` there.

## Input format

The input file name is fixed as `requests.log`. Each record is expected to contain five space-separated fields:

```text
TIMESTAMP METHOD ENDPOINT STATUS LATENCY_MICROSECONDS
```

For example:

```text
2025-01-15T13:42:18.123456Z GET /api/items 200 1842
```

The reference parser expects timestamps accepted by `%Y-%m-%dT%H:%M:%S.%f%z`, including a fractional second and timezone, and parses status and latency as integers. The fast parser is optimized for UTC `Z` timestamps and three-byte status codes. A format it cannot safely process causes a retry through the reference parser. Keep records on one line and separate fields with single spaces for compatibility with the reference parser.

## Output format

`summary.txt` contains one line per hour and endpoint, sorted by hour and endpoint:

```text
YYYY-MM-DDTHH:00:00Z ENDPOINT COUNT P50 P95 P99 SERVER_ERRORS ERROR_PPM
```

Latency percentiles are selected by nearest-rank indexing: the rank is `ceil(percentile × count / 100)`. Latencies are in the same units as the input (microseconds in the example). `SERVER_ERRORS` counts status values from 500 through 599, and `ERROR_PPM` is the integer floor of `SERVER_ERRORS * 1,000,000 / COUNT`.

## Python API

### `main(data_dir, out_dir)`

Runs the fast processor, using the reference processor for unsupported input or after a fast-path failure. `data_dir` is the directory containing `requests.log`; `out_dir` is where `summary.txt` is written. Returns `None` on success. File and parsing errors can propagate if the fallback also fails.

### `fast_main(data_dir, out_dir, single=False)`

Runs the optimized implementation. `single=True` disables multiprocessing. Returns `None` and writes `summary.txt`. It can raise the internal `Fallback` signal when input does not match fast-path assumptions, `WorkerFailed` for a child-process failure, and ordinary OS/runtime exceptions for I/O or resource failures. These exception classes are internal implementation details.

### `reference_main(data_dir, out_dir)`

Runs the straightforward Python implementation. It accepts `data_dir` and `out_dir` as above and returns `None`. Malformed rows, invalid timestamps or numeric fields, and file errors raise the corresponding Python exceptions (for example `ValueError`, `UnicodeDecodeError`, or `OSError`).

### Other functions

The module also defines parsing and computation helpers such as `parse_chunk`, `process_range`, `line_bounds`, `cpu_budget`, `ranks_of`, and `_percentile`. Names beginning with `_` and low-level helpers are not intended as stable public API. `Fallback` and `WorkerFailed` are internal control-flow exceptions.

## Examples

### Run from the command line

```sh
python data_processor.py --data ./sample-data --out ./results
```

### Create a sample log and process it from Python

```python
from pathlib import Path
import data_processor

Path("sample-data").mkdir(exist_ok=True)
Path("sample-data/requests.log").write_text(
    "2025-01-15T13:42:18.123456Z GET /api/items 200 1842\n"
    "2025-01-15T13:43:02.654321Z GET /api/items 503 2301\n"
    "2025-01-15T14:01:09.000001Z POST /api/items 201 3100\n",
    encoding="utf-8",
)

data_processor.main("sample-data", "results")
print(Path("results/summary.txt").read_text(encoding="utf-8"))
```

The first two records aggregate into the 13:00 hour for `/api/items`; the third is a separate 14:00-hour group.

## Performance controls

- `A_INGEST_WORKERS` sets the requested worker count for the fast path; it is clamped to at least one and otherwise defaults to the available CPU budget (including container CPU quota where detectable).
- `A_INGEST_CHUNK` sets the fast parser's read chunk size in bytes (default: 262144).
- Small input files are processed with one worker. The fast implementation parallelizes parsing and, for sufficiently large workloads, percentile calculation.

## Error handling and edge cases

- Empty input produces an empty `summary.txt`.
- Unsupported input encountered by the fast parser triggers the reference implementation. If a worker/runtime failure occurs, `main` retries the fast path in single-process mode, then tries the reference implementation.
- The reference parser expects exactly five fields per line, valid timestamp strings, and integer status/latency fields. Malformed content raises standard parsing exceptions; there is no per-record skip or error report.
- Negative latency values are accepted by the reference implementation as integers. The fast implementation stores values in an unsigned integer array and falls back for values outside that representation.
- A missing or unreadable `requests.log`, or an unwritable output directory, results in a file-system exception if retries cannot complete.
- The fast path assumes the file remains stable while being read. It falls back if reads indicate the file changed or a chunk does not meet its parsing assumptions.
