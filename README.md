# Request Log Ingest Summary

This project contains a Python log processor that reads `requests.log`, groups requests by hour and endpoint, calculates latency percentiles and 5xx error statistics, and writes the results to `summary.txt`.

## Usage

```bash
python a_ingest_fast.py [--data DATA_DIR] [--out OUTPUT_DIR]
```

The defaults are `/data` for the input directory and `/out` for the output directory. The script looks for `requests.log` in the input directory and writes `summary.txt` to the output directory. The input is expected to contain one request per line, with five space-separated fields:

```text
timestamp method endpoint status latency_us
```

Example:

```text
2025-01-15T13:24:10.123Z GET /health 200 840
```

The output has one row per hour and endpoint:

```text
YYYY-MM-DDTHH:00:00Z endpoint count p50 p95 p99 errors errors_per_million
```

Percentiles use the nearest-rank convention: for percentile `p`, the selected one-based rank is `ceil(p * count / 100)`. `errors_per_million` is the integer floor of `errors * 1,000,000 / count`.

## Code structure

### Configuration and helpers

The constants at the top set filenames, timestamp expectations, read chunk size, sorting piece size, and task limit. `A_INGEST_CHUNK` overrides the chunk size; `A_INGEST_WORKERS` can set the worker count. `Fallback` indicates that data does not fit the fast path's assumptions, while `WorkerFailed` represents a worker process failure.

### Phase 1: parsing and aggregation

- `parse_chunk` tokenizes a chunk, checks that it has five fields per line, and chooses a processing path based on timestamp ordering and hour distribution.
- `_add_segment` aggregates records that share an hour, grouping latency values by endpoint.
- `_count_errors` counts 5xx responses, with a shortcut for three-byte status fields and a general conversion path for other widths.
- `_add_general` handles chunks containing multiple hours by grouping each record by hour and endpoint.
- `process_range` reads a byte range in chunks while preserving whole lines.
- `line_bounds` divides the input into ranges whose boundaries align to line starts.

The phase-one accumulator is keyed by `(hour, endpoint)` and stores an error count plus latency arrays.

### Worker and CPU helpers

`cpu_budget` estimates available parallelism, including cgroup CPU limits where available. `spawn` runs a function in a forked process and returns its bytes through a pipe. `collect` gathers worker results and detects fallback or worker failures.

### Phase 2: percentile calculation

`ranks_of` combines latency buffers for a group and finds the p50, p95, and p99 values. Depending on group size and distinct-value count, it sorts all values, counts values with `_histogram_ranks`, or sorts bounded-size runs and searches for the requested ranks.

### Driver and reference implementation

`fast_main` coordinates reading, parsing, aggregation, percentile calculation, and output writing. Parsing and percentile work may run in child processes. `main` tries the fast path first, retries it in single-process mode after other fast-path errors, and uses `reference_main` when the fast path signals `Fallback` or the single-process retry fails.

`reference_main` is the simpler implementation: it parses timestamps with `datetime.strptime`, groups values, sorts each group's latencies, and writes the same summary format. `_parse_args` handles command-line arguments.

## Assumptions and uncertainties

- The parser expects exactly five whitespace-separated fields per line. It does not use the method field after parsing it.
- The fast path checks timestamp shape on only a few sample records in each chunk. Its hour grouping uses the first 13 timestamp bytes, so it assumes timestamps have a consistent textual timezone representation.
- `_ts_ok` requires timestamps to end in `Z`; the reference parser accepts timezone offsets described by `TIMESTAMP_FORMAT`. Inputs with other offsets may use the fallback. The fast path does not convert offsets to UTC before grouping.
- The error-count shortcut assumes a three-byte status beginning with `5` represents a 5xx response. The general path converts status values to integers and checks the inclusive range 500–599.
- Latencies are stored in an unsigned integer array (`array("I")`). Values that cannot be represented cause the fast path to fall back.
- The file is expected to remain unchanged while processing. A detected short read causes fallback, but the code does not lock or snapshot the input.
- Parallel processing depends on `os.fork()` and may behave differently on platforms that do not support it.
- The output file is written directly rather than through a temporary file and atomic replacement.

These points describe behavior visible in the code; confirm the intended log format and deployment platform before treating them as guaranteed input or runtime requirements.
## API reference

`data_processor.py` does not define a `DataProcessor` class. Its public API consists of module-level functions. The signatures below include inferred type hints for documentation; the source code itself does not declare these annotations. Names beginning with `_` are internal helpers and are not listed here.

### `main(data_dir, out_dir)`

```python
main(data_dir: str | os.PathLike, out_dir: str | os.PathLike) -> None
```

- `data_dir`: Directory containing `requests.log`.
- `out_dir`: Directory in which to write `summary.txt`; created if necessary.
- **Returns:** `None`. Runs the fast implementation, falling back to the reference implementation when needed.
- **Potential exceptions:** Exceptions raised by the reference implementation can propagate, including `OSError` for file operations, `UnicodeDecodeError` for invalid UTF-8, and `ValueError` for malformed records, timestamps, or numeric fields. The function catches fast-path exceptions to perform its retries.

### `fast_main(data_dir, out_dir, single=False)`

```python
fast_main(
    data_dir: str | os.PathLike,
    out_dir: str | os.PathLike,
    single: bool = False,
) -> None
```

- `data_dir`: Directory containing `requests.log`.
- `out_dir`: Directory in which to write `summary.txt`.
- `single`: If true, avoid multiprocessing for parsing and percentile work.
- **Returns:** `None`. Parses and aggregates requests, calculates percentiles, and writes the summary.
- **Potential exceptions:** `Fallback` when input data does not meet fast-path assumptions; `WorkerFailed` when a child worker fails; `OSError` for file, pipe, or process operations; and other exceptions from parsing, conversion, or allocation. `main` handles these exceptions by retrying or using the reference implementation.

### `reference_main(data_dir, out_dir)`

```python
reference_main(data_dir: str | os.PathLike, out_dir: str | os.PathLike) -> None
```

- `data_dir`: Directory containing `requests.log`.
- `out_dir`: Directory in which to write `summary.txt`.
- **Returns:** `None`. Parses records with Python's datetime and integer conversion, sorts each group, and writes the summary.
- **Potential exceptions:** `OSError` for file operations; `UnicodeDecodeError` for invalid UTF-8; `ValueError` for malformed timestamps, numeric values, or lines that do not contain exactly five fields; and `TypeError` for incompatible argument types.

### `parse_chunk(buf, acc)`

```python
parse_chunk(buf: bytes, acc: dict) -> None
```

- `buf`: Bytes containing complete log lines in the expected five-field format.
- `acc`: Mutable aggregation dictionary keyed by `(hour_bytes, endpoint_bytes)`.
- **Returns:** `None`; updates `acc` in place with error counts and latency arrays.
- **Potential exceptions:** `Fallback` when token counts or sampled timestamp structure do not match the fast path's assumptions. Invalid argument or data types can raise `TypeError`; malformed values may raise conversion exceptions, though some latency conversion failures are converted to `Fallback`.

### `process_range(fd, a, b, acc)`

```python
process_range(fd: int, a: int, b: int, acc: dict) -> None
```

- `fd`: Open file descriptor for the input log.
- `a`: Start byte offset, inclusive.
- `b`: End byte offset, exclusive; task boundaries should align to whole lines.
- `acc`: Mutable aggregation dictionary passed to `parse_chunk`.
- **Returns:** `None`; reads and parses the requested range, updating `acc`.
- **Potential exceptions:** `Fallback` if reads are short or complete line boundaries cannot be established; `OSError` from file reads; and `TypeError` for invalid arguments.

### `line_bounds(fd, size, ntasks)`

```python
line_bounds(fd: int, size: int, ntasks: int) -> list[int]
```

- `fd`: Open file descriptor for the input file.
- `size`: File size in bytes.
- `ntasks`: Number of requested partitions.
- **Returns:** `ntasks + 1` offsets from zero through `size`; interior offsets are advanced to line boundaries.
- **Potential exceptions:** `OSError` from `os.pread`; `TypeError` for incompatible arguments. The implementation does not explicitly validate that `ntasks` is positive.

### `cpu_budget()`

```python
cpu_budget() -> int
```

- **Parameters:** None.
- **Returns:** Cached estimate of the available CPU count, at least 1. It checks `A_INGEST_WORKERS`, CPU affinity/count, and supported cgroup quota files.
- **Potential exceptions:** `ValueError` if `A_INGEST_WORKERS` is set to a non-integer string. Errors reading optional cgroup files are caught internally.

### `ranks_of(pieces)`

```python
ranks_of(pieces: Iterable[bytes]) -> tuple[int, int, int, int]
```

- `pieces`: Iterable of byte buffers containing unsigned integers in the platform's `array("I")` representation.
- **Returns:** `(count, p50, p95, p99)`, where percentiles use the nearest-rank convention.
- **Potential exceptions:** `TypeError` for unsupported inputs; `ValueError` if a buffer's byte length is not a multiple of the integer array element size; and memory-related exceptions for very large inputs.

### `spawn(fn)`

```python
spawn(fn: Callable[[], bytes]) -> tuple[int, int]
```

- `fn`: Zero-argument callable expected to return a bytes-like result.
- **Returns:** `(pid, read_fd)`: child process ID and the parent's pipe read descriptor.
- **Potential exceptions:** `OSError` if pipe creation or `fork` fails; `AttributeError` on platforms without `os.fork`. Exceptions inside the child are represented by its exit status and are reported later by `collect`.

### `collect(children)`

```python
collect(children: list[tuple[int, int]]) -> list[bytes]
```

- `children`: `(pid, read_fd)` pairs returned by `spawn`.
- **Returns:** Child output buffers in the same order as `children`.
- **Potential exceptions:** `Fallback` if any child reports that input requires the reference path; `WorkerFailed` if a child exits unsuccessfully for another reason; `OSError` from pipe reads or process waiting.

## API notes

- `Fallback` and `WorkerFailed` are public exception classes used by the processing API; they are not methods. `Fallback` signals that the reference algorithm should process the input. `WorkerFailed` signals an unexpected child-process failure.
- The module has no `DataProcessor` class. If callers need an object-oriented API, one is not provided by this implementation.
- Command-line argument parsing is performed by the private `_parse_args` helper and is not part of the documented public API.

## Standalone numeric data examples

`data_processor.py` processes request logs; it does not provide numeric batch-ingestion, transformation, or CSV APIs. The following independent example uses Python's standard library to demonstrate those operations. Save it as `numeric_example.py` and run `python numeric_example.py`. It writes `numbers.csv` in the current directory.

```python
import csv
from statistics import mean, median, pstdev


def ingest_numbers(*values):
    """Ingest individual values and normalize them to floats."""
    return [float(value) for value in values]


def ingest_batches(batches):
    """Flatten batches of values and normalize them to floats."""
    return [float(value) for batch in batches for value in batch]


def summarize(values):
    """Compute count, min, max, mean, median, and population stddev."""
    if not values:
        raise ValueError("cannot summarize an empty collection")
    return {
        "count": len(values),
        "min": min(values),
        "max": max(values),
        "mean": mean(values),
        "median": median(values),
        "population_stddev": pstdev(values),
    }


def filter_and_transform(values, minimum, scale, offset):
    """Keep values >= minimum, then apply y = scale*x + offset."""
    return [scale * value + offset for value in values if value >= minimum]


def export_csv(path, values):
    """Write values to a CSV file with a header row."""
    with open(path, "w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["value"])
        writer.writerows([value] for value in values)


def import_csv(path):
    """Read floats from the CSV file's value column."""
    with open(path, newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)
        if reader.fieldnames is None or "value" not in reader.fieldnames:
            raise ValueError("CSV must contain a 'value' column")
        return [float(row["value"]) for row in reader]


if __name__ == "__main__":
    # Ingest scalar values and batches, then combine them.
    values = ingest_numbers(1, 2.5, 4) + ingest_batches([[3, 5], [8, 13]])
    print("Ingested:", values)

    # Calculate summary statistics.
    print("Summary:", summarize(values))

    # Keep values >= 4 and apply y = 2*x + 1.
    transformed = filter_and_transform(
        values, minimum=4, scale=2, offset=1
    )
    print("Filtered and transformed:", transformed)

    # Export to CSV, import it again, and display the restored values.
    export_csv("numbers.csv", transformed)
    restored = import_csv("numbers.csv")
    print("Imported from CSV:", restored)
```

The example raises `ValueError` for an empty summary input, a CSV without a `value` column, or invalid numeric text. File operations may raise `OSError` (including `FileNotFoundError` when reading a missing file).

## Empty input, invalid data, and file I/O

### Empty datasets

- An empty `requests.log` produces an empty `summary.txt`. The fast path skips parsing when the file size is zero; the reference path also writes no rows when there are no records.
- `parse_chunk(b"", acc)` returns without modifying the accumulator.
- Percentile calculation is used for populated groups; there is no separate empty-group result.

### Invalid data and types

The parser expects five whitespace-separated fields per request: timestamp, method, endpoint, status, and latency.

- A malformed field count or an unsuitable sampled timestamp makes the fast parser raise `Fallback`, after which `main()` uses the reference parser.
- Invalid timestamps, non-integer status values, and non-integer latencies can raise `ValueError` in the reference parser. Errors from the final reference attempt can propagate to the caller.
- The fast path stores latency values as unsigned integers. Values that cannot be converted or represented, including negative or too-large values, trigger fallback; the reference path may handle some values differently.
- For three-byte status fields, the fast path checks whether the first byte is `5`. For other widths it converts the status to an integer and checks the 500–599 range. Malformed three-byte statuses may therefore be handled differently by the fast and reference paths.
- The module does not comprehensively validate Python argument types. Direct calls with incompatible objects may raise built-in exceptions such as `TypeError`.

`main()` retries fast-path failures once in single-process mode, then calls the reference parser if that retry fails. Errors from the reference parser are not caught by another fallback layer.

### File I/O behavior

- The input is `<data_dir>/requests.log`. The fast path opens it as a file descriptor and reads byte ranges; the reference path opens it as UTF-8 text. The fast path closes its descriptor in a `finally` block.
- The output is `<out_dir>/summary.txt`. The output directory is created when needed, and an existing summary file is overwritten.
- Output is written directly to its destination rather than through a temporary file and atomic replacement. An I/O failure during writing may leave a partial output file.
- File and directory errors can raise `OSError` subclasses such as `FileNotFoundError` and `PermissionError`. After the fast-path retries, an I/O error from the final reference attempt can propagate.
- The code assumes the input file remains unchanged during processing. A detected short read triggers fallback, but the file is not locked or snapshotted.
