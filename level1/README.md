# Request Log Processor

`data_processor.py` reads HTTP request logs and writes hourly statistics for each
endpoint: request count, latency percentiles, HTTP 5xx count, and error rate.
It uses chunked parsing, compact integer arrays, and optional worker processes
to process large files, with a simpler reference implementation as a fallback.

> `INSTRUCTIONS.md` describes a numerical `DataProcessor` class, but the supplied
> source file implements this log processor instead. It has no `DataProcessor`
> class, mean/variance methods, filtering or transformation API, or CSV storage.
> This README documents the supplied implementation.

## Requirements and setup

Only Python's standard library is required; there are no packages to install.
The module imports `gc`, `marshal`, `os`, `sys`, `array`, `bisect`, and
`itertools`, plus `collections` and `datetime` inside helper functions. It does
not use the `math`, `csv`, or `typing` dependencies listed in the instructions.

Use Python 3.9 or newer for the complete optimized implementation
(`os.waitstatus_to_exitcode` was added in 3.9). The reference implementation
supports Python 3.7+, including timestamps ending in `Z`. The optimized path
requires Unix facilities such as `os.pread` and, for multiple workers,
`os.fork`. On Windows, `main()` falls back to the reference implementation.

Run from this directory:

```sh
python data_processor.py --data ./data --out ./out
```

The input is always `requests.log` inside the data directory. The output is
always `summary.txt` inside the output directory. Both directories can be
absolute or relative paths. The defaults are `/data` and `/out`.
`--data=./data` and `--out=./out` are also accepted. Run
`python data_processor.py --help` for usage; the help text currently displays
the older script name `a_ingest_fast.py`.

## Input and output formats

Each input line has five fields, separated by a single space:

```text
2026-11-03T09:10:00.000000Z GET /health 200 100
2026-11-03T09:20:00.000000Z GET /health 503 300
2026-11-03T09:30:00.000000Z POST /health 200 200
```

The fields are timestamp, HTTP method, endpoint, status code, and integer
latency in microseconds. The method is read but does not affect grouping.
Use valid UTC timestamps with fractional seconds and a trailing `Z` for
consistent behavior across both implementations. The reference parser also
accepts numeric timezone offsets, but does not convert them to UTC before
formatting the output with a literal `Z`.

The example produces:

```text
2026-11-03T09:00:00Z /health 3 200 300 300 1 333333
```

Output fields are:

| Field | Meaning |
| --- | --- |
| Hour | Timestamp rounded down to the hour |
| Endpoint | Request endpoint |
| Count | Number of requests in the group |
| p50 | Median latency using the nearest-rank rule |
| p95 | 95th-percentile latency |
| p99 | 99th-percentile latency |
| Errors | Count of status codes from 500 through 599 |
| Error rate | Integer parts per million: `errors * 1_000_000 // count` |

Percentiles select the value at one-based rank `ceil(percentile * count / 100)`
from sorted latencies; they do not interpolate. Rows are sorted by hour and
endpoint. Output is space-delimited text, not CSV, and has no header.

## API reference

The source has no type annotations. Types below describe expected arguments.
All functions without a leading underscore are listed here; most are internal
processing helpers. Prefer `main()` for normal use.

| Signature | Parameters | Return value |
| --- | --- | --- |
| `main(data_dir, out_dir)` | Input and output directory paths (`str` or path-like) | `None`; writes the summary, retrying with single-process and reference processing as needed |
| `fast_main(data_dir, out_dir, single=False)` | Directory paths; `single` (`bool`) disables worker processes | `None`; writes the summary using optimized processing |
| `reference_main(data_dir, out_dir)` | Directory paths | `None`; writes the summary using straightforward text parsing and sorting |
| `cpu_budget()` | No arguments | Positive `int` worker budget, cached after the first call |
| `parse_chunk(buf, acc)` | `buf`: complete log lines as `bytes`; `acc`: mutable accumulator dictionary | `None`; adds `(hour_bytes, endpoint_bytes)` keys with `[error_count, array('I')]` values |
| `process_range(fd, a, b, acc)` | Open input file descriptor (`int`), start/end byte offsets (`int`), accumulator (`dict`); boundaries must enclose complete lines | `None`; accumulates records from `[a, b)` |
| `line_bounds(fd, size, ntasks)` | File descriptor, byte size, positive task count (all `int`) | `list[int]` of `ntasks + 1` offsets; internal boundaries follow newlines and may repeat |
| `spawn(fn)` | Zero-argument callable returning bytes-like data | `(pid, read_fd)` as two integers in the parent; the child executes the callable and exits |
| `collect(children)` | Sequence of `(pid, read_fd)` pairs from `spawn()` | `list[bytes]` in child-list order; closes read descriptors and waits for children |
| `ranks_of(pieces)` | List of bytes-like buffers containing native `array('I')` latency data for one nonempty group | `(count, p50, p95, p99)`, all integers |

`Fallback` and `WorkerFailed` are public exception classes derived from
`Exception`. `Fallback` signals that optimized processing cannot handle the
input. `WorkerFailed` signals a nonzero worker exit other than the dedicated
fallback exit code.

### Exceptions and side effects

- `main()` catches fast-path exceptions and retries; exceptions from the final
  reference attempt propagate. It disables cyclic garbage collection globally
  with `gc.disable()` and does not re-enable it.
- File operations can raise `OSError` subclasses, including `FileNotFoundError`
  and `PermissionError`. Output directories are created automatically; an
  existing `summary.txt` is overwritten. Writes are not atomic.
- `reference_main()` can raise `ValueError` for malformed timestamps, incorrect
  field counts, or non-integer status/latency fields, and `UnicodeDecodeError`
  for invalid UTF-8. Incorrect argument types can raise `TypeError`.
- `fast_main()`, `parse_chunk()`, and `process_range()` can raise `Fallback`
  when their input assumptions fail. Conversion errors such as `ValueError`
  may also propagate directly. Unsupported OS functions can raise
  `AttributeError`; worker collection can raise `WorkerFailed`.
- `spawn()` and `collect()` can raise OS errors related to pipes, forking,
  reading, or waiting. Child exceptions become exit codes rather than being
  re-raised with their original traceback.
- `ranks_of()` requires at least one latency. Empty data raises `IndexError`;
  invalid buffer lengths can raise `ValueError`, and invalid buffer types can
  raise `TypeError`. Buffers must use the local machine's native array format.
- Helper functions assume valid descriptors, offsets, task counts, and
  accumulator shapes; they are not general-purpose validated APIs.

## Runnable examples

Run these examples from `level1`, where `data_processor.py` can be imported.
This complete example creates input, runs the processor, and reads its summary
without leaving files behind:

```python
from pathlib import Path
from tempfile import TemporaryDirectory
from data_processor import main

with TemporaryDirectory() as directory:
    root = Path(directory)
    data_dir = root / "data"
    out_dir = root / "out"
    data_dir.mkdir()
    records = [
        "2026-11-03T09:10:00.000000Z GET /health 200 100",
        "2026-11-03T09:20:00.000000Z GET /health 503 300",
        "2026-11-03T09:30:00.000000Z POST /health 200 200",
    ]
    (data_dir / "requests.log").write_text(
        "\n".join(records) + "\n", encoding="utf-8"
    )
    main(data_dir, out_dir)
    summary = (out_dir / "summary.txt").read_text(encoding="utf-8")
    assert summary == "2026-11-03T09:00:00Z /health 3 200 300 300 1 333333\n"
    print(summary, end="")
```

To calculate percentiles directly from a batch of integer latencies:

```python
from array import array
from data_processor import ranks_of

latencies = array("I", [100, 300, 200])
latencies.append(400)
latencies.extend([500, 600])
count, p50, p95, p99 = ranks_of([latencies.tobytes()])
assert (count, p50, p95, p99) == (6, 300, 600, 600)
```

There are no filtering, linear transformation, or CSV import/export methods
in this source. Use the log-file workflow above for ingestion and persistence.

## Configuration

| Environment variable | Default | Purpose |
| --- | --- | --- |
| `A_INGEST_CHUNK` | `262144` bytes | Parsing chunk size; set to a positive integer before importing the module |
| `A_INGEST_WORKERS` | Detected CPU budget | Worker count, clamped to at least one; read when the CPU budget is first requested |

Automatic CPU detection considers available CPUs and Linux cgroup quotas.
Small workloads run sequentially regardless of the worker budget. Invalid
integer settings can raise `ValueError`; chunk-size parsing happens at import
time, before `main()` can apply its fallback handling.

## Error handling and edge cases

- An empty input file produces an empty summary file.
- A missing input file is an error; it is not treated as an empty dataset.
- Input need not be sorted. Repeated records count as separate requests;
  different HTTP methods for the same endpoint share a group.
- Optimized latency storage uses native unsigned integers (`array('I')`,
  normally 32-bit). Negative or out-of-range values trigger fallback; the
  reference implementation accepts arbitrary Python integers, including
  negative latencies, without domain validation.
- Validation differs between paths. The optimized parser samples timestamp
  structure and, for three-byte statuses, counts those beginning with `5`.
  It is not a strict input validator. Supply valid three-digit numeric statuses
  and consistently formatted UTC timestamps.
- Blank lines, extra spaces, or malformed fields may trigger fallback and then
  fail reference parsing. The reference parser requires exactly five
  single-space-separated fields per line.
- Input must remain unchanged during processing. Some short reads trigger
  fallback, but processing does not take a consistent snapshot of a changing
  file.
- Latencies are retained in memory; chunked input does not make memory usage
  independent of dataset size. Worker failures can trigger retries, but
  sufficient memory is still required for reference processing.
