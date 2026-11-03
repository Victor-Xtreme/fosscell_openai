# Request Log Ingestor

## 1. Overview

`data_processor.py` is a command-line program for summarizing large request logs. It reads `requests.log`, groups requests by hour and endpoint, calculates request counts and latency percentiles, counts HTTP 5xx responses, and writes the results to `summary.txt`.

The module has a chunked binary fast path that can distribute parsing and percentile work across forked worker processes. If data does not fit the fast parser's assumptions, it uses a reference implementation that processes the file as UTF-8 text. The code does **not** define a `DataProcessor` class and does not implement numerical dataset CSV import/export; its input and output are request-log and summary text files.

> **Task specification mismatch:** `INSTRUCTIONS.md` asks for a `DataProcessor`
> class, Python 3.7+, and `math`/`csv`/`typing` APIs. None of those appear in
> `data_processor.py`; the actual program uses a different interface and
> requires Python 3.9+ on POSIX. This README documents the checked-in program
> rather than describing methods that do not exist.

## 2. Requirements & Setup

- **Python:** 3.9 or newer. The code calls `os.waitstatus_to_exitcode`, which is available starting with Python 3.9.
- **Platform:** POSIX, such as Linux. The implementation uses `os.fork` and `os.pread`; the parallel fast path does not run on native Windows.
- **Dependencies:** Python standard library only: `gc`, `marshal`, `os`, `sys`, `array`, `bisect`, `itertools`, plus `collections` and `datetime` in selected paths.

The script expects a file named `requests.log` in the data directory. Run it from the directory containing the script:

```bash
python data_processor.py --data ./data --out ./out
```

This reads `./data/requests.log`, creates `./out` if needed, and writes `./out/summary.txt`. Without options, the directories default to `/data` and `/out`.

```text
--data DIRECTORY   Directory containing requests.log
--out DIRECTORY    Directory for summary.txt
-h, --help         Display usage
```

Options may also be written as `--data=./data` and `--out=./out`. The module can be imported, but it is designed as a script; its primary entry point is `main(data_dir, out_dir)`.

Optional environment variables:

- `A_INGEST_WORKERS`: worker count; values are clamped to a minimum of one. When unset, the program detects available CPUs and attempts to honor Linux cgroup CPU quotas.
- `A_INGEST_CHUNK`: fast-path read chunk size in bytes; defaults to `262144`. Set it to a positive integer.

## 3. API Reference

The source does not use Python type annotations. The signatures and types below describe its implementation. Functions whose names do not begin with an underscore are listed here; several are low-level helpers intended for use by the driver rather than typical callers.

The `DataProcessor` constructor and methods named in `INSTRUCTIONS.md`
(`__init__`, `add_data`, `batch_add`, the calculation methods,
`get_summary_statistics`, `filter_data`, `transform_data`, `export_to_csv`,
and `load_from_csv`) are **not defined** in this source file. Their parameters,
return values, exceptions, and CSV examples cannot be documented as working API
without changing the program itself.

### `main(data_dir, out_dir)`

```python
main(data_dir, out_dir) -> None
```

Runs the processing pipeline and writes `summary.txt`. It temporarily disables garbage collection, tries `fast_main`, retries the fast path in single-process mode after an unexpected exception, and finally invokes `reference_main` if the retry fails. It restores garbage collection to its prior enabled/disabled state before returning or propagating an exception. Returns `None`.

**Parameters:** `data_dir` and `out_dir` are path strings accepted by `os.path.join` and filesystem operations.

**Exceptions:** Exceptions raised by the final reference pass propagate, including `OSError` for file access, `ValueError` for malformed timestamps or integer fields, and `UnicodeDecodeError` for invalid UTF-8 input.

### `fast_main(data_dir, out_dir, single=False)`

```python
fast_main(data_dir, out_dir, single=False) -> None
```

Runs the optimized binary parser, aggregation, percentile calculation, and report writer. If `single` is true, it avoids worker parallelism. Returns `None`.

**Parameters:** `data_dir` is the input directory; `out_dir` is the output directory; `single` is a boolean selecting serial execution.

**Exceptions:** `Fallback` indicates input or read conditions requiring the reference implementation. `WorkerFailed` indicates an unsuccessful child process. Filesystem operations may raise `OSError` (including `FileNotFoundError`); serialization, parsing, and process operations may raise their underlying exceptions. `main` handles fast-path exceptions as described above.

### `reference_main(data_dir, out_dir)`

```python
reference_main(data_dir, out_dir) -> None
```

Reads and processes the entire input as UTF-8 text, then writes the summary file. Returns `None`.

**Parameters:** `data_dir` is the input directory; `out_dir` is the output directory.

**Exceptions:** `OSError` for input/output failures; `ValueError` for malformed records, timestamps, or integer fields; `UnicodeDecodeError` for invalid UTF-8. These errors are not suppressed when this is the final processing attempt.

### `cpu_budget()`

```python
cpu_budget() -> int
```

Returns a cached estimate of available worker processes, with a minimum of one. It honors `A_INGEST_WORKERS` when set; otherwise it checks CPU availability and supported cgroup quota files.

**Exceptions:** `ValueError` if `A_INGEST_WORKERS` is not an integer. The driver may respond by using the reference implementation.

### `parse_chunk(buf, acc)`

```python
parse_chunk(buf, acc) -> None
```

Parses a bytes buffer containing complete request records and updates `acc`, a dictionary keyed by `(hour_bytes, endpoint_bytes)`. Each value accumulates an error count and unsigned integer latency values. An empty buffer is ignored.

**Parameters:** `buf` is `bytes`; `acc` is a mutable accumulator dictionary in the internal format expected by the implementation.

**Exceptions:** `Fallback` when token counts or fast-path assumptions fail; conversion and type errors can propagate for malformed data or incompatible arguments.

### `process_range(fd, a, b, acc)`

```python
process_range(fd, a, b, acc) -> None
```

Reads and parses the file byte range `[a, b)` from file descriptor `fd`, updating `acc`. Reads are aligned to whole lines by the task-boundary logic.

**Parameters:** `fd` is an open file descriptor usable by `os.pread`; `a` and `b` are byte offsets; `acc` is the accumulator dictionary.

**Exceptions:** `OSError` for read failures; `Fallback` for short reads or data requiring the reference path; exceptions from `parse_chunk` may propagate.

### `line_bounds(fd, size, ntasks)`

```python
line_bounds(fd, size, ntasks) -> list[int]
```

Returns file offsets that divide `size` bytes into `ntasks` ranges, with interior boundaries aligned to line starts. The returned list has `ntasks + 1` entries, beginning at zero and ending at `size`.

**Parameters:** `fd` is a readable file descriptor supporting `os.pread`; `size` is the file size in bytes; `ntasks` is the desired positive task count.

**Exceptions:** `OSError` for file read errors; invalid arguments may cause arithmetic or API errors.

### `spawn(fn)` and `collect(children)`

```python
spawn(fn) -> tuple[int, int]
collect(children) -> list[bytes]
```

`spawn` forks a child that executes zero-argument callable `fn` and returns `(pid, read_fd)` in the parent. `collect` consumes `(pid, read_fd)` pairs, returning each child's bytes in order.

**Exceptions:** `OSError` for pipe, fork, descriptor, or wait operations; `Fallback` if a worker exits with the fallback status; `WorkerFailed` if a worker exits unsuccessfully for another reason.

### `ranks_of(pieces)`

```python
ranks_of(pieces) -> tuple[int, int, int, int]
```

Combines byte buffers holding native unsigned integer (`array("I")`) values and returns `(count, p50, p95, p99)` using nearest-rank percentiles.

**Parameters:** `pieces` is an iterable of compatible byte buffers. This is a low-level helper; normal callers should use `main` or `fast_main`.

**Exceptions:** `ValueError` for byte buffers whose lengths are incompatible with `array.frombytes`; `IndexError` if called with no values. The driver calls it only for non-empty groups.

### `Fallback` and `WorkerFailed`

These exception classes coordinate fallback and worker retry behavior. `Fallback` signals that the reference parser should be used. `WorkerFailed` signals that a child process failed for a reason other than the input-data fallback condition. They are not result values.

## 4. Code Examples

### Create a sample log and run the command

The following shell commands create a minimal input and process it:

```bash
mkdir -p data out
cat > data/requests.log <<'EOF'
2025-03-08T10:15:30.123456Z GET /api/items 200 1250
2025-03-08T10:18:02.000000Z POST /api/items 503 8700
2025-03-08T11:02:10.250000Z GET /api/items 200 900
EOF
python data_processor.py --data ./data --out ./out
cat ./out/summary.txt
```

Example output (one row per hour and endpoint):

```text
2025-03-08T10:00:00Z /api/items 2 1250 8700 8700 1 500000
2025-03-08T11:00:00Z /api/items 1 900 900 900 0 0
```

### Import and call from Python

```python
from data_processor import main

main("./data", "./out")
```

The call reads `./data/requests.log` and writes `./out/summary.txt`. For ordinary use, call `main`; the chunk parsing, process management, and rank helpers are lower-level implementation interfaces.

## 5. Error Handling & Edge Cases

- **Empty input file:** The program creates an empty `summary.txt`.
- **Malformed records:** The fast parser detects some unsupported shapes and falls back to the reference parser. The reference parser expects exactly five single-space-separated fields on each line. Missing fields, extra spaces, invalid timestamps, and invalid integer fields can raise `ValueError` (or `UnicodeDecodeError` for invalid UTF-8).
- **Status codes:** The intended format is a numeric HTTP status. The fallback counts values from 500 through 599. In the optimized path, three-byte status fields are classified by whether their first byte is `5`; use valid three-digit numeric codes to ensure fast-path and fallback results agree.
- **Latency values:** Latencies are integer microseconds. The fast path stores values as unsigned `array("I")`; negative or too-large values trigger fallback, whose Python integer path can handle them.
- **Timestamps:** Use timestamps with fractional seconds and a timezone, preferably UTC `Z` timestamps. The fast parser is specialized for UTC-style timestamps ending in `Z`; the reference parser uses `%Y-%m-%dT%H:%M:%S.%f%z`.
- **File I/O:** Missing or unreadable input and unwritable output raise filesystem exceptions if the final reference attempt also fails. The fast path treats a short read as a fallback condition because the file may have changed while it was being processed. Keep the input stable during a run.
- **Worker failures:** A fast-path worker failure causes a serial retry through `main`; if that fails, `main` attempts the reference implementation. Failures in the final reference attempt propagate to the caller.
- **Platform limitations:** The parallel fast path requires POSIX `fork` and `pread`. Run it in a supported POSIX environment.
- **Chunk configuration:** `A_INGEST_CHUNK` must be a positive integer. Invalid values can fail during module import; zero is not a valid chunk size.
- **Output values:** Percentiles are nearest-rank values at `ceil(p * n / 100)`. Error rate is integer-truncated parts per million: `errors * 1_000_000 // count`.
