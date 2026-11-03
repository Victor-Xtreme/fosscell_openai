import gc
import marshal
import os
import sys
from array import array
from bisect import bisect_left, bisect_right
from itertools import compress

INPUT_NAME = "requests.log"
OUTPUT_NAME = "summary.txt"
TIMESTAMP_FORMAT = "%Y-%m-%dT%H:%M:%S.%f%z"

CHUNK = int(os.environ.get("A_INGEST_CHUNK", 256 * 1024))  # bytes per cache-sized chunk
PIECE = 1 << 20  # max elements sorted at once inside one group
MAX_TASKS = 8192  # task queue pipe buffer limit

HOUR_LEN = 13  # len("YYYY-MM-DDTHH")
HOUR_END = b"\x7f"  # sorts after every byte that can follow the hour prefix

# ASCII byte constants
BYTE_NEWLINE = 10
BYTE_Z = 90
BYTE_T = 84
BYTE_DASH = 45
BYTE_COLON = 58
BYTE_FIVE = 53

# Process exit status codes
EXIT_SUCCESS = 0
EXIT_ERROR = 1
EXIT_FALLBACK = 3

# Status translation table: first-digit -> 1 for '5', else 0
_T5 = bytes(1 if i == BYTE_FIVE else 0 for i in range(256))


class Fallback(Exception):
    """The input is not what the fast path assumes; use the reference algorithm."""


class WorkerFailed(Exception):
    """A child process died for a reason unrelated to the data (e.g., out of memory)."""


# --------------------------------------------------------------------------- #
# Phase 1: Parsing Helpers                                                    #
# --------------------------------------------------------------------------- #

def _ts_ok(timestamp_bytes):
    """Validate ISO timestamp format ('YYYY-MM-DDTHH:MM:SS.f...Z')."""
    return (
        len(timestamp_bytes) >= 22
        and timestamp_bytes[-1] == BYTE_Z
        and timestamp_bytes[10] == BYTE_T
        and timestamp_bytes[4] == BYTE_DASH
        and timestamp_bytes[7] == BYTE_DASH
        and timestamp_bytes[13] == BYTE_COLON
    )


def _filter_5xx_statuses(statuses):
    """Return set of non-standard status byte strings representing HTTP 5xx errors."""
    return {status for status in set(statuses) if 500 <= int(status) <= 599}


def _count_errors(endpoints, statuses, line_count):
    """Return {endpoint: number of 5xx errors} for one hour segment."""
    joined_statuses = b"".join(statuses)
    
    if len(joined_statuses) == 3 * line_count:  # Standard 3-byte status codes
        first_digits = joined_statuses[0::3]
        five_count = first_digits.count(b"5")
        if not five_count:
            return {}
        selection = first_digits.translate(_T5)
    else:  # Non-standard width status codes
        s5_set = _filter_5xx_statuses(statuses)
        if not s5_set:
            return {}
        selection = bytes(map(s5_set.__contains__, statuses))
        five_count = selection.count(1)
        if not five_count:
            return {}

    errors = {}
    if five_count * 6 < line_count:
        idx = selection.find(1)
        while idx >= 0:
            ep = endpoints[idx]
            errors[ep] = errors.get(ep, 0) + 1
            idx = selection.find(1, idx + 1)
    else:
        for ep in compress(endpoints, selection):
            errors[ep] = errors.get(ep, 0) + 1
    return errors


def _update_accumulator(accumulator, key, error_count, latency_tokens):
    """Convert string latency tokens to uint32 array and accumulate into state dict."""
    try:
        latencies_array = array("I", map(int, latency_tokens))
    except (OverflowError, ValueError):
        raise Fallback

    acc_entry = accumulator.get(key)
    if acc_entry is None:
        accumulator[key] = [error_count, latencies_array]
    else:
        acc_entry[0] += error_count
        acc_entry[1].extend(latencies_array)


def _add_segment(hour, endpoints, statuses, latencies, accumulator):
    """Ingest lines that all share the same hour label."""
    endpoint_latencies = {}
    for ep, lat in zip(endpoints, latencies):
        endpoint_latencies.setdefault(ep, []).append(lat)

    errors_by_endpoint = _count_errors(endpoints, statuses, len(endpoints))
    
    for ep, tokens in endpoint_latencies.items():
        key = (hour, ep)
        err_count = errors_by_endpoint.get(ep, 0)
        _update_accumulator(accumulator, key, err_count, tokens)


def _add_general(timestamps, endpoints, statuses, latencies, accumulator):
    """Ingest chunk with arbitrary mixed hours."""
    s5_set = _filter_5xx_statuses(statuses)
    grouped_data = {}
    
    for ts, ep, status, lat in zip(timestamps, endpoints, statuses, latencies):
        key = (ts[:HOUR_LEN], ep)
        record = grouped_data.setdefault(key, [[], 0])
        record[0].append(lat)
        if status in s5_set:
            record[1] += 1

    for key, (tokens, err_count) in grouped_data.items():
        _update_accumulator(accumulator, key, err_count, tokens)


def parse_chunk(buffer, accumulator):
    """Parse a buffer containing complete log lines."""
    tokens = buffer.split()
    total_tokens = len(tokens)
    if not total_tokens:
        return

    line_count = buffer.count(b"\n")
    if buffer[-1] != BYTE_NEWLINE:
        line_count += 1

    if total_tokens != 5 * line_count:
        raise Fallback

    timestamps = tokens[0::5]
    if not (_ts_ok(timestamps[0]) and _ts_ok(timestamps[-1]) and _ts_ok(timestamps[line_count >> 1])):
        raise Fallback

    endpoints = tokens[2::5]
    statuses = tokens[3::5]
    latencies = tokens[4::5]
    del tokens

    if timestamps == sorted(timestamps):
        # Time-ordered: process one segment per hour found via bisection
        idx = 0
        while idx < line_count:
            hour = timestamps[idx][:HOUR_LEN]
            if timestamps[-1][:HOUR_LEN] == hour:
                end_idx = line_count
            else:
                end_idx = bisect_left(timestamps, hour + HOUR_END, idx, line_count)

            if idx == 0 and end_idx == line_count:
                _add_segment(hour, endpoints, statuses, latencies, accumulator)
            else:
                _add_segment(
                    hour,
                    endpoints[idx:end_idx],
                    statuses[idx:end_idx],
                    latencies[idx:end_idx],
                    accumulator,
                )
            idx = end_idx
        return

    # Unordered chunk: check if all sampled timestamps fall into the same hour
    hour = timestamps[0][:HOUR_LEN]
    quarter = line_count >> 2
    if (
        timestamps[quarter][:HOUR_LEN] == hour
        and timestamps[line_count >> 1][:HOUR_LEN] == hour
        and timestamps[line_count - 1 - quarter][:HOUR_LEN] == hour
        and timestamps[-1][:HOUR_LEN] == hour
        and min(timestamps)[:HOUR_LEN] == hour
        and max(timestamps)[:HOUR_LEN] == hour
    ):
        _add_segment(hour, endpoints, statuses, latencies, accumulator)
        return

    _add_general(timestamps, endpoints, statuses, latencies, accumulator)


def process_range(fd, start_pos, end_pos, accumulator):
    """Parse complete lines in byte range [start_pos, end_pos)."""
    pos = start_pos
    while pos < end_pos:
        rem = end_pos - pos
        if rem <= CHUNK + (CHUNK >> 1):
            buf = os.pread(fd, rem, pos)
            if len(buf) != rem:
                raise Fallback  # File changed during read
        else:
            size = CHUNK
            while True:
                buf = os.pread(fd, size, pos)
                cut = buf.rfind(b"\n") + 1
                if cut:
                    break
                if size >= rem:
                    raise Fallback
                size = min(size * 2, rem)
            if cut != len(buf):
                buf = buf[:cut]
        pos += len(buf)
        parse_chunk(buf, accumulator)


# --------------------------------------------------------------------------- #
# Phase 2: System & Process Plumbing                                         #
# --------------------------------------------------------------------------- #

_CPUS = None


def cpu_budget():
    global _CPUS
    if _CPUS is None:
        _CPUS = _cpu_budget()
    return _CPUS


def _cpu_budget():
    env_workers = os.environ.get("A_INGEST_WORKERS")
    if env_workers:
        return max(1, int(env_workers))

    proc_count_fn = getattr(os, "process_cpu_count", None)
    if proc_count_fn is not None:
        n_cpus = proc_count_fn() or 1
    else:
        try:
            n_cpus = len(os.sched_getaffinity(0))
        except AttributeError:
            n_cpus = os.cpu_count() or 1

    # Container CPU quota enforcement (cgroups v2 & v1)
    try:
        with open("/sys/fs/cgroup/cpu.max") as fh:
            quota, period = fh.read().split()[:2]
        if quota != "max":
            n_cpus = min(n_cpus, max(1, -(-int(quota) // int(period))))
    except (OSError, ValueError):
        try:
            with open("/sys/fs/cgroup/cpu/cpu.cfs_quota_us") as fh:
                quota = int(fh.read())
            with open("/sys/fs/cgroup/cpu/cpu.cfs_period_us") as fh:
                period = int(fh.read())
            if quota > 0 and period > 0:
                n_cpus = min(n_cpus, max(1, -(-quota // period)))
        except (OSError, ValueError):
            pass

    return max(1, n_cpus)


def _advise(fd):
    try:
        os.posix_fadvise(fd, 0, 0, os.POSIX_FADV_SEQUENTIAL)
    except (AttributeError, OSError):
        pass


def _open_and_advise(path):
    """Open input file read-only and set sequential access advice."""
    fd = os.open(path, os.O_RDONLY)
    _advise(fd)
    return fd


def line_bounds(fd, size, ntasks):
    """Return ntasks+1 offsets where every inner bound begins a line."""
    bounds = [0]
    for k in range(1, ntasks):
        p = max(size * k // ntasks - 1, bounds[-1])
        while True:
            buf = os.pread(fd, 65536, p)
            if not buf:
                p = size
                break
            idx = buf.find(b"\n")
            if idx >= 0:
                p += idx + 1
                break
            p += len(buf)
        bounds.append(min(p, size))
    bounds.append(size)
    return bounds


def spawn(worker_fn):
    """Fork process to run worker_fn and transmit output via pipe."""
    read_fd, write_fd = os.pipe()
    pid = os.fork()
    if pid:
        os.close(write_fd)
        return pid, read_fd

    exit_code = EXIT_ERROR
    try:
        os.close(read_fd)
        blob = worker_fn()
        view = memoryview(blob)
        while len(view):
            view = view[os.write(write_fd, view):]
        exit_code = EXIT_SUCCESS
    except Fallback:
        exit_code = EXIT_FALLBACK
    except BaseException:
        pass
    finally:
        os._exit(exit_code)


def collect(children):
    """Collect outputs from worker processes or raise on failures."""
    outputs = []
    exit_codes = []
    for pid, fd in children:
        with os.fdopen(fd, "rb", buffering=0) as fh:
            outputs.append(fh.read())
        _, status = os.waitpid(pid, 0)
        exit_codes.append(os.waitstatus_to_exitcode(status))

    if EXIT_FALLBACK in exit_codes:
        raise Fallback
    if any(exit_codes):
        raise WorkerFailed
    return outputs


# --------------------------------------------------------------------------- #
# Phase 3: Rank Calculation                                                   #
# --------------------------------------------------------------------------- #

def ranks_of(pieces):
    """Calculate percentile ranks (p50, p95, p99) from uint32 data buffers."""
    latencies_array = array("I")
    for piece in pieces:
        latencies_array.frombytes(piece)

    n_samples = len(latencies_array)
    target_ranks = (
        (50 * n_samples + 99) // 100,
        (95 * n_samples + 99) // 100,
        (99 * n_samples + 99) // 100,
    )

    # Check for low cardinality distributions suitable for counting histogram
    is_low_cardinality = (
        n_samples >= 20000 or n_samples > PIECE
    ) and (len(set(latencies_array[:4096])) * 5 < 2 * 4096)

    if is_low_cardinality:
        hist_res = _histogram_ranks(latencies_array, target_ranks)
        if hist_res is not None:
            return (n_samples,) + hist_res

    if n_samples <= PIECE:
        sorted_vals = latencies_array.tolist()
        sorted_vals.sort()
        return (
            n_samples,
            sorted_vals[target_ranks[0] - 1],
            sorted_vals[target_ranks[1] - 1],
            sorted_vals[target_ranks[2] - 1],
        )

    # Large datasets: sort PIECE elements at a time, then bisect values across runs
    runs = []
    for i in range(0, n_samples, PIECE):
        chunk_vals = latencies_array[i:i + PIECE].tolist()
        chunk_vals.sort()
        runs.append(array("I", chunk_vals))
    del latencies_array

    lo_bound = min(r[0] for r in runs)
    hi_bound = max(r[-1] for r in runs)
    ranks_out = []

    for rank in target_ranks:
        lo, hi = lo_bound, hi_bound
        while lo < hi:
            mid = (lo + hi) >> 1
            count = sum(bisect_right(run, mid) for run in runs)
            if count >= rank:
                hi = mid
            else:
                lo = mid + 1
        ranks_out.append(lo)

    return (n_samples,) + tuple(ranks_out)


def _histogram_ranks(latencies_array, target_ranks):
    """Calculate ranks using frequency counting for low-cardinality data."""
    from collections import Counter

    counts = Counter()
    step = 1 << 21
    for i in range(0, len(latencies_array), step):
        counts.update(latencies_array[i:i + step])
        if len(counts) > 2_000_000:
            return None

    ranks_out = []
    cumulative_count = 0
    target_idx = 0

    for val in sorted(counts):
        cumulative_count += counts[val]
        while target_idx < 3 and cumulative_count >= target_ranks[target_idx]:
            ranks_out.append(val)
            target_idx += 1
        if target_idx == 3:
            break

    return tuple(ranks_out)


# --------------------------------------------------------------------------- #
# Execution Drivers                                                           #
# --------------------------------------------------------------------------- #

def fast_main(data_dir, out_dir, single=False):
    path = os.path.join(data_dir, INPUT_NAME)
    fd = _open_and_advise(path)
    
    try:
        file_size = os.fstat(fd).st_size
        groups = {}  # (hour, endpoint) -> [error_count, [data_buffers]]
        
        if file_size:
            workers = 1 if single else cpu_budget()
            if file_size < (1 << 21):
                workers = 1

            task_size = max(1 << 21, min(64 << 20, file_size // (workers * 8)))
            ntasks = max(1, min(MAX_TASKS, -(-file_size // task_size)))
            bounds = line_bounds(fd, file_size, ntasks)
            tasks = [(bounds[i], bounds[i + 1]) for i in range(ntasks) if bounds[i + 1] > bounds[i]]
            workers = min(workers, len(tasks))

            if workers == 1:
                accumulator = {}
                for start, end in tasks:
                    process_range(fd, start, end, accumulator)
                for key, (err_count, arr) in accumulator.items():
                    groups[key] = [err_count, [arr.tobytes()]]
            else:
                pipe_read_fd, pipe_write_fd = os.pipe()
                os.write(
                    pipe_write_fd,
                    b"".join(i.to_bytes(4, "little") for i in range(len(tasks))),
                )
                os.close(pipe_write_fd)

                def worker_task():
                    worker_fd = _open_and_advise(path)
                    try:
                        acc = {}
                        while True:
                            raw_idx = os.read(pipe_read_fd, 4)
                            if len(raw_idx) < 4:
                                break
                            start, end = tasks[int.from_bytes(raw_idx, "little")]
                            process_range(worker_fd, start, end, acc)
                        return marshal.dumps({k: (v[0], v[1].tobytes()) for k, v in acc.items()})
                    finally:
                        os.close(worker_fd)

                children = [spawn(worker_task) for _ in range(workers)]
                os.close(pipe_read_fd)

                for blob in collect(children):
                    for key, (err_count, data_bytes) in marshal.loads(blob).items():
                        group_entry = groups.get(key)
                        if group_entry is None:
                            groups[key] = [err_count, [data_bytes]]
                        else:
                            group_entry[0] += err_count
                            group_entry[1].append(data_bytes)
                    del blob
    finally:
        os.close(fd)

    # Phase 2: Compute percentile ranks
    keys = sorted(groups)
    total_bytes = sum(len(p) for g in groups.values() for p in g[1])
    results = {}
    n_procs = 1 if single else min(cpu_budget(), len(keys))

    if n_procs <= 1 or total_bytes < (1 << 20):
        for key in keys:
            results[key] = ranks_of(groups[key][1])
    else:
        sorted_keys_by_size = sorted(keys, key=lambda k: -sum(len(p) for p in groups[k][1]))
        work_bins = [[] for _ in range(n_procs)]
        bin_loads = [0] * n_procs

        for key in sorted_keys_by_size:
            min_bin_idx = bin_loads.index(min(bin_loads))
            work_bins[min_bin_idx].append(key)
            bin_loads[min_bin_idx] += sum(len(p) for p in groups[key][1]) or 1

        def make_rank_worker(assigned_keys):
            def run():
                return marshal.dumps({k: ranks_of(groups[k][1]) for k in assigned_keys})
            return run

        children = [spawn(make_rank_worker(b)) for b in work_bins]
        for blob in collect(children):
            results.update(marshal.loads(blob))

    output_lines = []
    for key in keys:
        hour, endpoint = key
        err_count = groups[key][0]
        n_count, p50, p95, p99 = results[key]
        ppm = err_count * 1_000_000 // n_count
        output_lines.append(
            b"%b:00:00Z %b %d %d %d %d %d %d\n"
            % (hour, endpoint, n_count, p50, p95, p99, err_count, ppm)
        )

    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, OUTPUT_NAME), "wb") as f:
        f.write(b"".join(output_lines))


# --------------------------------------------------------------------------- #
# Reference Algorithm (Fallback Path)                                         #
# --------------------------------------------------------------------------- #

def _percentile(sorted_values, p):
    n = len(sorted_values)
    rank = (p * n + 99) // 100
    return sorted_values[rank - 1]


def reference_main(data_dir, out_dir):
    from datetime import datetime

    latencies = {}
    errors = {}
    
    with open(os.path.join(data_dir, INPUT_NAME), encoding="utf-8") as f:
        for line in f:
            ts, _, endpoint, status, latency_us = line.rstrip("\n").split(" ")
            when = datetime.strptime(ts, TIMESTAMP_FORMAT)
            hour = when.replace(minute=0, second=0, microsecond=0)
            key = (hour, endpoint)
            
            latencies.setdefault(key, []).append(int(latency_us))
            errors.setdefault(key, 0)
            
            if 500 <= int(status) <= 599:
                errors[key] += 1

    output_lines = []
    for key in sorted(latencies):
        hour, endpoint = key
        values = sorted(latencies[key])
        count = len(values)
        err_count = errors[key]
        ppm = err_count * 1_000_000 // count
        output_lines.append(
            f"{hour.strftime('%Y-%m-%dT%H:00:00Z')} {endpoint} {count} "
            f"{_percentile(values, 50)} {_percentile(values, 95)} {_percentile(values, 99)} "
            f"{err_count} {ppm}\n"
        )

    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, OUTPUT_NAME), "w", encoding="utf-8", newline="\n") as f:
        f.write("".join(output_lines))


def main(data_dir, out_dir):
    gc.disable()
    try:
        fast_main(data_dir, out_dir)
    except Fallback:
        reference_main(data_dir, out_dir)
    except Exception:
        try:
            fast_main(data_dir, out_dir, single=True)
        except Exception:
            reference_main(data_dir, out_dir)


def _parse_args(argv):
    data_dir, out_dir = "/data", "/out"
    arg_iter = iter(argv)
    
    for arg in arg_iter:
        if arg in ("--data", "--out"):
            try:
                val = next(arg_iter)
            except StopIteration:
                sys.exit(f"error: argument {arg}: expected one argument")
            if arg == "--data":
                data_dir = val
            else:
                out_dir = val
        elif arg.startswith("--data="):
            data_dir = arg[7:]
        elif arg.startswith("--out="):
            out_dir = arg[6:]
        elif arg in ("-h", "--help"):
            print("usage: a_ingest_fast.py [--data DATA] [--out OUT]")
            sys.exit(0)
        else:
            sys.exit(f"error: unrecognized argument: {arg}")
            
    return data_dir, out_dir


if __name__ == "__main__":
    main(*_parse_args(sys.argv[1:]))
    sys.exit(0)
