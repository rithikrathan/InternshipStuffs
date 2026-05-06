#!/usr/bin/env python3
"""Capture PlatformIO serial monitor output to a log file."""

import os
import subprocess
import sys
import time
from datetime import datetime

LOG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
LOG_FILE = os.path.join(LOG_DIR, f"serial_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log")

os.makedirs(LOG_DIR, exist_ok=True)

print(f"[monitor] Logging to: {LOG_FILE}")
print("[monitor] Press Ctrl+C to stop\n")

proc = subprocess.Popen(
    ["pio", "device", "monitor", "--baud", "115200"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    universal_newlines=True,
    bufsize=1,
)

with open(LOG_FILE, "w") as f:
    f.write(f"# HomeAutomation Serial Log\n")
    f.write(f"# Started: {datetime.now().isoformat()}\n")
    f.write(f"# Board: ESP-12E (ETSS401006)\n")
    f.write(f"# Baud: 115200\n")
    f.write("# " + "=" * 60 + "\n\n")
    try:
        for line in proc.stdout:
            line = line.rstrip()
            print(line)
            f.write(line + "\n")
            f.flush()
    except KeyboardInterrupt:
        print("\n[monitor] Stopping...")
        proc.terminate()
        proc.wait()
        print(f"[monitor] Log saved to: {LOG_FILE}")
        sys.exit(0)
