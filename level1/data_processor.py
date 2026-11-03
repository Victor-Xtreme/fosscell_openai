import gc
import marshal
import os
import sys
from array import array
from datetime import date
from bisect import bisect_left, bisect_right
from itertools import compress

INPUT_NAME = "requests.log"
OUTPUT_NAME = "summary.txt"
TIMESTAMP_FORMAT = "%Y-%m-%dT%H:%M:%S.%f%z"   # only used by the fallback

CHUNK = int(os.environ.get("A_INGEST_CHUNK", 256 * 1024))   # bytes per cache-sized chunk
if CHUNK <= 0:
    raise ValueError("A_INGEST_CHUNK must be a positive integer")
PIECE = 1 << 20          # max elements sorted at once inside one group
MAX_TASKS = 8192         # the task queue is a pipe; 8192 * 4 bytes fits its 64 KiB buffer

HOUR_LEN = 13            # len("YYYY-MM-DDTHH")
HOUR_END = b"\x7f"       # sorts after every byte that can follow the hour prefix

# status first-digit -> 1 for '5', else 0
_T5 = bytes(1 if i == 53 else 0 for i in range(256))


class Fallback(Exception):
    """The input is not what the fast path assumes; use the reference algorithm."""


class WorkerFailed(Exception):
    """A child process died for a reason unrelated to the data (e.g. out of memory)."""


# --------------------------------------------------------------------------- #
# phase 1: parsing                                                            #
# --------------------------------------------------------------------------- #

def _ts_ok(t):
    """Return whether t has the complete UTC timestamp format used by fast path.

    The reference parser also accepts numeric UTC offsets; those timestamps
    deliberately use the fallback because the fast path groups by the literal
    hour prefix.
    """
    if not 22 <= len(t) <= 27 or t[-1:] != b"Z":
        return False
    if (t[4:5] != b"-" or t[7:8] != b"-" or t[10:11] != b"T"
            or t[13:14] != b":" or t[16:17] != b":" or t[19:20] != b"."):
        return False
    fraction = t[20:-1]
    if not 1 <= len(fraction) <= 6:
        return False
    digits = (t[:4] + t[5:7] + t[8:10] + t[11:13] + t[14:16]
              + t[17:19] + fraction)
    if not digits.isdigit():
        return False
    try:
        year, month, day = int(t[:4]), int(t[5:7]), int(t[8:10])
        hour, minute, second = int(t[11:13]), int(t[14:16]), int(t[17:19])
        date(year, month, day)
    except ValueError:
        return False
    return hour < 24 and minute < 60 and second < 60


def _count_errors(eps, sts, m):
    """Return {endpoint: number of 5xx} for one hour-segment of m lines."""
    js = b"".join(sts)
    if len(js) == 3 * m:                       # every status is exactly 3 bytes
        first = js[0::3]
        c = first.count(b"5")
        if not c:
            return {}
        sel = first.translate(_T5)
    else:                                      # unusual widths: exact int() semantics
        s5 = {s for s in set(sts) if 500 <= int(s) <= 599}
        if not s5:
            return {}
        sel = bytes(map(s5.__contains__, sts))
        c = sel.count(1)
        if not c:
            return {}
    errs = {}
    get = errs.get
    if c * 6 < m:
        k = sel.find(b"\x01")
        while k >= 0:
            e = eps[k]
            errs[e] = get(e, 0) + 1
            k = sel.find(b"\x01", k + 1)
    else:
        for e in compress(eps, sel):
            errs[e] = get(e, 0) + 1
    return errs
