# Request Log Processor

This README describes the code currently in `data_processor.py`.

> **Scope note:** The [Level 1 task sheet](./INSTRUCTIONS.md) describes a numerical `DataProcessor` class with statistics, filtering, transformations, and CSV methods. The current `data_processor.py` does not define that class or those methods. It implements a request-log aggregation command-line program instead. This README documents the implementation that is actually present; it does not claim unsupported APIs.

## Overview

The program reads a file named `requests.log`, groups requests by hour and endpoint, and writes a summary file named `summary.txt`. For each group, the summary contains the request count, p50/p95/p99 latency in microseconds, the number of 5xx responses, and the 5xx rate in parts per million (ppm).

The script has two processing paths:

- A chunked, optimized path that can use multiple processes on supported Unix systems.
- A simpler reference path used when the optimized path cannot handle the input or fails.

No third-party packages are required.

## Requirements and setup

The implementation uses only the Python standard library. It imports `gc`, `marshal`, `os`, `sys`, `array`, `bisect`, and `itertools`; `datetime` and `collections.Counter` are imported inside functions.

For the optimized multi-process path, use Python 3.9 or newer on a Unix-like system. That path uses `os.fork()` and `os.pread()`; the worker collection code also calls `os.waitstatus_to_exitcode()`, added in Python 3.9. The Level 1 task sheet says Python 3.7+, but the current implementation's compatibility with that version and with Windows has not been established. The program attempts slower fallback paths when optimized processing fails. See the Python documentation for [`os.fork()`](https://docs.python.org/3/library/os.html#os.fork) and [`os.waitstatus_to_exitcode()`](https://docs.python.org/3/library/os.html#os.waitstatus_to_exitcode).

From the repository root, create a data directory containing `requests.log`, then run:

~~~sh
python level1/data_processor.py --data ./data --out ./out
~~~

The output directory is created if needed. The command-line options are:

- `--data PATH`: directory containing `requests.log` (default: `/data`)
- `--out PATH`: directory where `summary.txt` is written (default: `/out`)
- `--help`: print usage

## Input format

The input file is plain text, with one request per line and five fields:

~~~text
timestamp method endpoint status latency_us
~~~

For consistent fast-path behavior, use one ASCII space between fields and timestamps in UTC with a trailing `Z`. For example:

~~~text
2025-03-08T10:15:30.123456Z GET /health 200 1200
2025-03-08T10:45:00.000000Z GET /health 500 1600
2025-03-08T10:30:00.000000Z POST /items 201 2300
~~~

The timestamp format used by the reference path is `%Y-%m-%dT%H:%M:%S.%f%z`. The method is required as a field but is not included in the aggregation. The endpoint is the grouping key. Status values are parsed as integers by the reference path; responses from 500 through 599 count as errors. Latencies are parsed as integer microseconds.

The current code does not validate the full input contract before processing. Keep each line to five fields, use numeric status and latency values, and use nonnegative latencies that fit in an unsigned 32-bit integer for the optimized path. Inputs that the fast path cannot represent may be retried by the reference path.

## Output format

Each output line has these whitespace-separated fields:

~~~text
hour_utc endpoint request_count p50_us p95_us p99_us error_count error_rate_ppm
~~~

Rows are sorted by hour and endpoint. Percentiles use the nearest-rank rule: the selected one-based rank is `ceil(percentile × count / 100)`. The error rate is integer-truncated: `error_count × 1,000,000 // request_count`.

For the sample input above, `out/summary.txt` contains:

~~~text
2025-03-08T10:00:00Z /health 2 1200 1600 1600 1 500000
2025-03-08T10:00:00Z /items 1 2300 2300 2300 0 0
~~~

The output timestamp is formatted with a literal `Z`. The program does not convert non-UTC input offsets to UTC before writing that label, so supply UTC timestamps to avoid misleading output.

## Processing structure

- `main(data_dir, out_dir)` is the orchestration entry point. It tries the optimized path first, then retries without multiple processes after other fast-path errors, and finally uses the reference path if needed.
- `fast_main(data_dir, out_dir, single=False)` divides the input into line-aligned ranges, parses chunks, aggregates each hour/endpoint group, and calculates percentiles. It can distribute parsing and ranking work across child processes.
- `reference_main(data_dir, out_dir)` reads and parses the log line by line, sorts each group's latency values, and writes the same summary format. It is simpler but may use substantially more memory for large logs.
- Parsing helpers such as `parse_chunk()`, `_add_segment()`, and `_add_general()` build group counts and latency buffers. `ranks_of()` and `_histogram_ranks()` calculate percentile ranks.
- `_parse_args(argv)` handles the command-line options. Running the file directly calls `main()`.

The available Python interface consists of module-level functions. There is no `DataProcessor` class, and no `calculate_mean()`, `filter_data()`, `transform_data()`, or CSV import/export methods in the current source.

## Runnable example

From the repository root, create `data/requests.log` with the three sample lines in the Input format section, then run:

~~~sh
python level1/data_processor.py --data ./data --out ./out
cat ./out/summary.txt
~~~

On Windows, use `py` instead of `python` if that is how Python is installed, and use a command that displays the contents of `out/summary.txt`.

## Error handling and edge cases

- An empty input file produces an empty `summary.txt`.
- The output directory is created automatically.
- The optimized path can raise an internal `Fallback` signal when an input chunk does not match its assumptions. The program then tries the reference implementation.
- If another optimized-path exception occurs, `main()` retries once in single-process mode before using the reference implementation.
- The reference path expects each nonempty line to split into exactly five space-separated fields. Missing files, malformed rows, nonnumeric status or latency values, and other file I/O errors can raise exceptions; the program does not convert all such failures into friendly error messages.
- The optimized path stores latencies in an unsigned integer array. Values outside that representation cause a fallback attempt; the reference path uses Python integers.
- There is no CSV handling, numeric dataset validation, filtering, or transformation behavior in the current implementation.

## Runtime tuning

Two environment variables affect the optimized path:

- `A_INGEST_WORKERS`: override the detected worker count with a positive integer.
- `A_INGEST_CHUNK`: set the parser chunk size in bytes (default: 262144).

The code also sets internal limits for tasks and per-piece sorting. For normal use, the defaults should be left unchanged.
