# Level 1 — Request Log Data Processor

## Overview

`data_processor.py` processes a request log and generates a summary grouped by **hour** and **endpoint**.

For each group, it calculates:

- Total number of requests
- 50th percentile (P50) latency
- 95th percentile (P95) latency
- 99th percentile (P99) latency
- Number of server errors (HTTP status codes 500–599)
- Error rate in parts per million (PPM)

The implementation has an optimized processing path that uses chunked file reads and, where available, multiple processes. It also includes a reference implementation that is used when the optimized path cannot handle the input.

## Project Structure

```text
level1/
├── data_processor.py
├── INSTRUCTIONS.md
└── awesome.md
```

The two Markdown files are shown here as expected Level 1 files; their contents and any additional project requirements should be documented once confirmed.

## Input

The program expects a file named:

```text
requests.log
```

By default, it looks for this file inside `/data`.

Each input line is expected to contain five space-separated fields:

```text
timestamp method endpoint status latency_us
```

Example:

```text
2026-11-03T10:30:45.123Z GET /api/users 200 450
2026-11-03T10:31:12.456Z POST /api/users 500 820
2026-11-03T11:02:01.789Z GET /api/items 200 120
```

Field descriptions:

| Field | Description |
|---|---|
| `timestamp` | Request timestamp |
| `method` | HTTP method, such as `GET` or `POST` |
| `endpoint` | Requested endpoint |
| `status` | HTTP response status code |
| `latency_us` | Request latency in microseconds |

The method is read from the input but is not used as a grouping key.

**Input assumptions:** The reference implementation expects exactly five fields separated by single spaces. The optimized path is designed around UTC timestamps ending in `Z`, with a fractional second component. Inputs outside the optimized path's assumptions may be handled by the reference implementation, provided they satisfy its parsing requirements.

## Output

The program writes its results to:

```text
summary.txt
```

By default, the output directory is `/out`.

Each output row represents one hour and one endpoint:

```text
hour endpoint count p50 p95 p99 errors error_ppm
```

Example:

```text
2026-11-03T10:00:00Z /api/users 2 450 820 820 1 500000
2026-11-03T11:00:00Z /api/items 1 120 120 120 0 0
```

Column descriptions:

| Column | Description |
|---|---|
| `hour` | Start of the hour |
| `endpoint` | Request endpoint |
| `count` | Total requests in the group |
| `p50` | 50th percentile latency |
| `p95` | 95th percentile latency |
| `p99` | 99th percentile latency |
| `errors` | Number of responses with status 500–599 |
| `error_ppm` | Error rate in parts per million |

The error rate is calculated as:

```text
error_ppm = errors * 1,000,000 // count
```

Percentiles use the nearest-rank method:

```text
rank = ceil(percentile * count / 100)
```

The value at that one-based rank in the sorted latency values is returned.

## Running the Program

From the repository root, run:

```bash
python level1/data_processor.py
```

The default paths are:

- Input directory: `/data`
- Output directory: `/out`

Custom directories can be supplied using command-line arguments:

```bash
python level1/data_processor.py --data ./data --out ./output
```

The program creates the output directory if it does not already exist.

To view the supported command-line options:

```bash
python level1/data_processor.py --help
```

## Processing Design

The implementation is organized into the following parts.

### 1. Parsing and grouping

- Reads the input file in chunks.
- Splits records into timestamp, endpoint, status, and latency fields.
- Groups latency values by hour and endpoint.
- Counts HTTP 5xx responses for each group.

### 2. Parallel processing

- Determines a worker count from available CPU resources and optional environment configuration.
- Splits the input into line-aligned ranges.
- Uses child processes to process ranges when parallel processing is selected.
- Combines partial results from workers.

### 3. Percentile calculation

- Combines latency values for each hour-endpoint group.
- Calculates P50, P95, and P99.
- Uses sorting for ordinary-sized groups and alternative strategies for certain larger or low-cardinality groups.

### 4. Fallback handling

If the optimized parser encounters input it cannot safely process, the program uses the reference implementation. If multiprocessing or the optimized execution path fails, it attempts a single-process optimized run before falling back to the reference implementation.

## Configuration

The implementation supports these environment variables:

| Variable | Purpose | Default |
|---|---|---|
| `A_INGEST_CHUNK` | Chunk size in bytes used when reading the input | `262144` (256 KiB) |
| `A_INGEST_WORKERS` | Explicit worker count for parallel processing | Automatically determined |

Example:

```bash
A_INGEST_CHUNK=524288 A_INGEST_WORKERS=2 python level1/data_processor.py
```

The multiprocessing implementation uses `os.fork()` and `os.pread()`. It is therefore intended for Unix-like environments that provide these operating-system features; it is not directly compatible with a standard native Windows Python environment.

## Current Scope and Points to Verify

The following details should be checked against the official Level 1 instructions or evaluator before treating them as fixed requirements:

- Maximum input file size and expected number of records
- Whether timestamps are always UTC and always end in `Z`
- Whether fractional seconds are mandatory
- Whether input fields can contain spaces or unusual whitespace
- Allowed latency range and whether negative values are possible
- Required behavior for malformed records
- Whether the output format and percentile convention match the evaluator's specification
- Required operating system and available CPU/memory limits
- Whether the method should be included in the grouping key

This README describes the behavior visible in the current implementation. It does not assume additional requirements that have not been confirmed.

## Notes

- The input filename is fixed as `requests.log`.
- The output filename is fixed as `summary.txt`.
- The output is sorted by the hour and endpoint keys.
- The implementation disables Python garbage collection in its main execution path.
