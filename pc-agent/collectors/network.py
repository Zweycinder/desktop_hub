import re
import subprocess
import time
from collections import deque

import psutil

_TIME_RE = re.compile(r"time[=<]([\d.]+)\s*ms", re.IGNORECASE)


def ping_once(target, timeout_ms=1000):
    """Returns round-trip ms, or None on loss. Uses the OS ping binary to
    avoid needing raw-socket/admin privileges."""
    try:
        out = subprocess.run(
            ["ping", "-n", "1", "-w", str(timeout_ms), target],
            capture_output=True, text=True, timeout=(timeout_ms / 1000) + 1,
        ).stdout
    except (subprocess.TimeoutExpired, OSError):
        return None
    match = _TIME_RE.search(out)
    return float(match.group(1)) if match else None


class PingStats:
    def __init__(self, target, window=20):
        self.target = target
        self.samples = deque(maxlen=window)

    def sample(self):
        rtt = ping_once(self.target)
        self.samples.append(rtt)

    def current(self):
        """Returns (ping_ms, jitter_ms, loss_pct) from the rolling window."""
        valid = [s for s in self.samples if s is not None]
        if not valid:
            return 0, 0, 100
        ping_ms = int(valid[-1])
        if len(valid) > 1:
            diffs = [abs(valid[i] - valid[i - 1]) for i in range(1, len(valid))]
            jitter_ms = int(sum(diffs) / len(diffs))
        else:
            jitter_ms = 0
        loss_pct = int((1 - len(valid) / len(self.samples)) * 100)
        return ping_ms, jitter_ms, loss_pct


class NetSpeed:
    def __init__(self):
        counters = psutil.net_io_counters()
        self._prev_sent = counters.bytes_sent
        self._prev_recv = counters.bytes_recv
        self._prev_t = time.monotonic()

    def sample(self):
        """Returns (download_KBs, upload_KBs)."""
        counters = psutil.net_io_counters()
        now = time.monotonic()
        dt = max(now - self._prev_t, 0.001)
        dl_kbs = int((counters.bytes_recv - self._prev_recv) / dt / 1024)
        ul_kbs = int((counters.bytes_sent - self._prev_sent) / dt / 1024)
        self._prev_sent = counters.bytes_sent
        self._prev_recv = counters.bytes_recv
        self._prev_t = now
        return max(dl_kbs, 0), max(ul_kbs, 0)
