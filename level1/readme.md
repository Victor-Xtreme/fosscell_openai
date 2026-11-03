HTTP Request Log Ingestor
A high-performance Python utility that reads an HTTP request log, groups
requests by hour and endpoint, calculates latency percentiles, counts
server errors, and writes a compact summary file.
> **Repository note:** The supplied Level 1 instructions describe a
> `DataProcessor` class and methods such as `calculate_mean()` and
> `export_to_csv()`. The supplied `data_processor.py` does not implement
> that API. It implements the log-ingestion utility documented here.
> This README intentionally documents the actual source code rather than
> describing methods that are not present.
Table of Contents
Overview
Requirements
Input Format
Output Format
How It Works
Running the Program
Configuration
Implementation Details
Error Handling and Edge Cases
Performance Notes
Limitations
License
Overview
The program analyzes request logs to produce one summary row for each
`(hour, endpoint)` group. For each group, it reports:
The number of requests.
The 50th percentile (P50) latency.
The 95th percentile (P95) latency.
The 99th percentile (P99) latency.
The number of HTTP 5xx responses.
The 5xx error rate in parts per million (PPM).
It is designed for large log files. It reads the input in chunks, can
distribute parsing and percentile calculations across multiple
processes, and uses a reference implementation when the optimized parser
cannot safely handle an input.
Requirements
Python: Python 3.7 or later.
Operating system: A POSIX-compatible operating system is
required for the multiprocessing fast path because the
implementation uses `os.fork()` and `os.pread()`. Linux is the
intended environment.
Dependencies: Standard library only. No third-party packages are
required.
The code imports standard-library modules including `gc`, `marshal`,
`os`, `sys`, `array`, `bisect`, and `itertools`. The fallback
implementation also uses `datetime`.
Input Format
By default, the program reads a file named `requests.log` from the data
directory.
Each non-empty record must contain exactly five whitespace-separated
fields:
``` text
timestamp method endpoint status latency_us
```
Example:
``` text
2026-11-03T10:15:23.123Z GET /api/items 200 1250
2026-11-03T10:15:24.456Z POST /api/items 503 8420
2026-11-03T10:16:01.010Z GET /health 200 320
```
Field descriptions:
---
Field                               Description
---
`timestamp`                         Timestamp in the form
`YYYY-MM-DDTHH:MM:SS.fractionZ`
`method`                            HTTP method, such as `GET` or
`POST`
`endpoint`                          Requested endpoint or path
`status`                            HTTP status code
`latency_us`                        Request latency in microseconds,
represented as an integer
The optimized parser is designed for timestamp-ordered or
arbitrary-order records using UTC timestamps ending in `Z`, and
non-negative latency values that fit in an unsigned 32-bit integer.
Inputs outside the fast path's assumptions may be processed by the
reference implementation if they are valid according to its parsing
rules.
Output Format
The program writes `summary.txt` into the output directory. Each line
has this format:
``` text
YYYY-MM-DDTHH:00:00Z endpoint count p50 p95 p99 errors error_rate_ppm
```
Example:
``` text
2026-11-03T10:00:00Z /api/items 2 1250 8420 8420 1 500000
2026-11-03T10:00:00Z /health 1 320 320 320 0 0
```
Columns:
---
Column                              Meaning
---
Hour                                Start of the hour represented by
the group
Endpoint                            Endpoint associated with the
requests
`count`                             Number of requests in the group
`p50`                               50th percentile latency in
microseconds
`p95`                               95th percentile latency in
microseconds
`p99`                               99th percentile latency in
microseconds
`errors`                            Number of responses with status
codes from 500 through 599
`error_rate_ppm`                    `errors * 1,000,000 // count`
Percentiles use the nearest-rank method. For a percentile `p` and a
group containing `n` values, the selected one-based rank is:
``` text
ceil(p * n / 100)
```
The implementation calculates this using integer arithmetic.
How It Works
The program has two main processing phases.
1. Parsing and aggregation
The input file is opened and its size is determined.
The file is divided into byte ranges aligned to line boundaries.
Each worker reads its assigned ranges in chunks.
Records are grouped by hour and endpoint.
Latencies are stored in unsigned integer arrays.
Responses with status codes from 500 to 599 are counted.
When multiple workers are available, the program uses child processes
and a task queue to parse separate file ranges. Each child returns its
partial aggregates to the parent process.
2. Percentile calculation
After parsing, the program combines the partial aggregates for each
group and calculates P50, P95, and P99.
It uses different strategies depending on the group size and the
distribution of latency values:
Smaller groups are converted to a list and sorted.
Groups with relatively few distinct values may use a counting
histogram.
Very large groups are sorted in smaller runs, then binary search is
used to find the values at the requested ranks.
The final rows are ordered by hour and endpoint before being written to
`summary.txt`.
Running the Program
Assuming the script is named `data_processor.py` and the directory
structure is:
``` text
project/
├── data_processor.py
├── data/
│   └── requests.log
└── out/
```
Run it with the default directories:
``` bash
python data_processor.py
```
The defaults are:
Input directory: `/data`
Output directory: `/out`
Input filename: `requests.log`
Output filename: `summary.txt`
To specify different directories:
``` bash
python data_processor.py --data ./data --out ./out
```
The equivalent `--option=value` syntax is also supported:
``` bash
python data_processor.py --data=./data --out=./out
```
To display the command-line usage:
``` bash
python data_processor.py --help
```
After a successful run, inspect:
``` text
./out/summary.txt
```
The output directory is created automatically if it does not exist.
Configuration
Two environment variables control performance-related settings.
---
Variable                                   Default Description
---
`A_INGEST_CHUNK`          `262144` bytes (256 KiB) Target read-chunk
size used while
parsing
`A_INGEST_WORKERS`             Detected CPU budget Overrides the number
of
parsing/aggregation
workers
Examples:
Set a larger chunk size:
``` bash
A_INGEST_CHUNK=1048576 python data_processor.py --data ./data --out ./out
```
Limit the worker count:
``` bash
A_INGEST_WORKERS=4 python data_processor.py --data ./data --out ./out
```
The worker count is also limited by the number of available tasks. The
program attempts to account for CPU affinity and Linux cgroup CPU quotas
when detecting the available CPU budget.
Implementation Details
Important constants
---
Constant                            Purpose
---
`INPUT_NAME`                        Input filename, `requests.log`
`OUTPUT_NAME`                       Output filename, `summary.txt`
`TIMESTAMP_FORMAT`                  Timestamp format used by the
reference parser
`CHUNK`                             Configurable byte chunk size
`PIECE`                             Maximum number of values sorted in
one run for large groups
`MAX_TASKS`                         Upper bound on the number of
parsing tasks
Main functions
---
Function                                       Purpose
---
`main(data_dir, out_dir)`                      Entry point; runs the optimized
path and manages fallback/retry
behavior
`fast_main(data_dir, out_dir, single=False)`   Runs the optimized parsing and
percentile pipeline
`parse_chunk(buf, acc)`                        Parses a chunk containing complete
log lines
`process_range(fd, a, b, acc)`                 Reads and processes a byte range
from the input file
`line_bounds(fd, size, ntasks)`                Calculates task boundaries aligned
to line endings
`cpu_budget()`                                 Returns the detected or configured
CPU worker budget
`ranks_of(pieces)`                             Calculates count, P50, P95, and P99
for a group's latency values
`_histogram_ranks(arr, rk)`                    Calculates requested ranks using
value counts when practical
`reference_main(data_dir, out_dir)`            Slower reference implementation
used as a fallback
`_percentile(sorted_values, p)`                Calculates a nearest-rank
percentile from sorted values
`_parse_args(argv)`                            Parses `--data` and `--out`
command-line arguments
Internal helpers
Functions prefixed with an underscore are implementation helpers, not a
stable public API. They include timestamp checks, error counting,
aggregation, process creation, and worker-result collection.
The script is intended to be run as a command-line program. It does not
define a `DataProcessor` class or a documented import-oriented class
API.
Error Handling and Edge Cases
The program uses two custom exceptions internally:
`Fallback`: signals that the optimized parser's assumptions are not
satisfied, so the reference implementation should be used.
`WorkerFailed`: signals that a child process failed for a reason
other than an input-format fallback.
The top-level `main()` function follows this recovery sequence:
Run the optimized implementation, which may use multiple processes.
If the optimized parser raises `Fallback`, run the reference
implementation.
If another exception occurs, retry the optimized implementation in
single-process mode.
If the single-process retry also fails, run the reference
implementation.
Other behaviors and edge cases:
Empty input file: produces an empty `summary.txt`.
Missing or unreadable input file: the file-opening error is not
converted into a user-friendly message by the script; it propagates
out of the top-level call.
Malformed records: the reference parser expects exactly five
space-separated fields and a timestamp matching `TIMESTAMP_FORMAT`.
Invalid records can raise parsing or conversion errors.
Non-numeric latency: integer conversion fails in the reference
implementation.
Negative or too-large latency: the optimized unsigned 32-bit
representation cannot store such values, so it requests fallback
processing.
Input changed while being read: short reads or inconsistent
chunk boundaries trigger fallback behavior.
Worker failure: the parent detects non-zero child exit codes and
attempts a single-process retry through `main()`.
The fallback implementation is not a general-purpose repair mechanism:
the input still needs to be valid for the reference parser.
Performance Notes
Chunked reads avoid loading the entire input file as one byte
string.
Line-aligned task boundaries prevent workers from splitting records.
Multiple processes can parse independent file ranges.
Aggregates use compact unsigned integer arrays for latency storage.
Histogram ranking can avoid sorting when a group has relatively few
distinct values.
Large groups are sorted in bounded-size runs to reduce the size of
individual sorting operations.
The implementation disables cyclic garbage collection while the main
processing routine runs.
Actual performance depends on file size, number of distinct endpoints
and hours, latency distribution, available CPU resources, and storage
speed. Increasing the worker count or chunk size is not guaranteed to
improve performance on every machine.
Limitations
The optimized implementation is platform-dependent because it uses
POSIX file operations and `fork()`.
The fast path assumes a specific log layout and timestamp
convention; it should not be treated as a fully validating log
parser.
The optimized latency storage uses unsigned 32-bit integers.
The program summarizes request logs; it does not implement the
`DataProcessor` numerical-dataset API described in the supplied
Level 1 instructions.
The program writes a summary text file, not a CSV file.
License
Add the license applicable to this repository here. No license is
specified by the supplied source file.
