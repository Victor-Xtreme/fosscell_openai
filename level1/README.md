# Request Log Ingestor

A command-line processor for large `requests.log` files. It groups request records by UTC hour and endpoint, then writes request counts, latency percentiles, HTTP 5xx counts, and an error rate in parts per million to `summary.txt`. The fast path parses binary chunks and can use multiple forked worker processes; a reference implementation is used when the input does not fit the fast parser's assumptions or when the parallel path fails.

## Requirements

- Python 3.9 or newer. The implementation uses `os.waitstatus_to_exitcode` and POSIX process/file APIs.
- POSIX operating system with `os.fork` and `os.pread` (for example, Linux). The multiprocessing fast path is not portable to native Windows.
- Standard library only: `gc`, `marshal`, `os`, `sys`, `array`, `bisect`, `itertools`, and (on selected paths) `collections` and `datetime`.

The program is a script, not a `DataProcessor` class or import-oriented library. Save it as `a_ingest_fast.py` (or use its actual filename) and run it with Python.

## Input format

The input filename is fixed as `requests.log` inside the data directory. Each line must contain five fields separated by single spaces:

```text
<timestamp> <method> <endpoint> <status> <latency_us>
```

Example:

```text
2025-03-08T10:15:30.123456+00:00 GET /api/items 200 1250
2025-03-08T10:18:02.000000+00:00 POST /api/items 503 8700
```

Timestamps must include fractional seconds and a timezone offset; the documented format is `%Y-%m-%dT%H:%M:%S.%f%z`. The fast parser is optimized for timestamps ending in `Z`, which represent UTC. Latencies are integer microseconds. Endpoints must not contain spaces because the input is split into five fields.

For reliable results, use valid timestamps and integer status and latency fields on every line. The fast path is optimized for three-character status codes and unsigned 32-bit latency values; unusual status widths or latencies outside that range cause the reference path to be used. The reference parser also expects exactly one space between fields.

## Run

```bash
python a_ingest_fast.py --data ./data --out ./out
```

This reads `./data/requests.log`, creates the output directory if needed, and writes `./out/summary.txt`. If options are omitted, the defaults are `/data` and `/out`.

Options can be provided as separate arguments or with `=`:

```text
--data DIRECTORY   Directory containing requests.log (default: /data)
--out DIRECTORY    Destination directory (default: /out)
-h, --help         Show usage
```

The worker count can be set with `A_INGEST_WORKERS` (minimum effective value is one). `A_INGEST_CHUNK` controls the fast parser's read chunk size in bytes and defaults to 262144. If `A_INGEST_WORKERS` is unset, the program detects available CPUs and attempts to honor a Linux cgroup CPU quota. Smaller input files are processed with one worker regardless.

## Output

`summary.txt` contains one row per `(hour, endpoint)`, sorted by hour and endpoint. Rows have space-separated fields:

```text
YYYY-MM-DDTHH:00:00Z endpoint request_count p50_us p95_us p99_us error_count error_rate_ppm
```

Percentiles use the nearest-rank definition: rank `ceil(p * n / 100)`, one-based, for `p50`, `p95`, and `p99`. The error rate is integer-truncated parts per million: `error_count * 1_000_000 // request_count`. A status is an error when its numeric value is from 500 through 599, inclusive.

For example, if a group has 10 requests, 2 5xx responses, and a p50 latency of 1250 microseconds, its row will include `10 ... 1250 ... 2 2000000` in the corresponding columns. An empty input file produces an empty `summary.txt`.

## Processing behavior

The fast path divides the file into whole-line ranges, parses chunks, groups latencies and error counts by hour and endpoint, and computes percentiles. It uses optimized sorting/counting strategies and may distribute parsing and percentile work across forked processes. `--help` displays the accepted CLI syntax.

If the fast parser detects an unsupported input shape or an out-of-range latency, it raises its internal `Fallback` signal and the reference algorithm reparses the full file. Other fast-path failures cause a single-process retry, followed by a reference-path attempt if that retry also fails. Errors from the final reference pass (including missing input files, invalid timestamps, malformed records, and invalid integer fields) are not suppressed.

## Operational notes

- The program disables Python garbage collection in `main()` and does not re-enable it before exit; this is appropriate for its one-shot command-line execution model.
- Input is read by file offsets. The file should remain unchanged during processing; a short read is treated as a reason to fall back.
- Parallel execution depends on POSIX `fork`, pipes, and `pread`. Use a POSIX environment for the intended fast path.
- The fallback parser expects UTF-8 text with exactly five single-space-separated fields per line. Malformed data can still fail in the fallback and will then raise its underlying exception.
