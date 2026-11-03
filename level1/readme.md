# Data Processor

`data_processor.py` is a high-performance log-processing program.

It reads HTTP request logs, groups requests by hour and endpoint, calculates latency percentiles and server-error statistics, and writes the results to a summary file.

## Features

- Processes large log files in chunks.
- Supports parallel processing on systems with `fork()`.
- Groups requests by hour and endpoint.
- Calculates p50, p95, and p99 latency.
- Counts HTTP 5xx errors.
- Calculates error rates in parts per million.
- Includes a slower reference implementation as a fallback.
- Supports configurable input and output directories.

## Requirements

- Python 3.7 or newer.
- The following standard-library modules are used:

```text
gc
marshal
os
sys
array
bisect
itertools
datetime
collections