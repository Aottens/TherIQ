#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
#
# TQ-09: receive EMS-ESP syslog (UDP) and append one line per message to a file.
#
# Why not `nc -ul`: EMS-ESP sends each syslog message as one UDP datagram
# without a trailing newline, so nc glues every message into one endless line.
# This script adds the newline and the Mac's wall-clock time (EMS-ESP has no
# clock without NTP, and NTP stays off: no internet traffic).
#
# Output line: <local time, ms>\t<syslog message exactly as received>
# Usage:       python3 tools/ems_syslog_capture.py <outfile> [port]   (default port 5514)
# Stop with Ctrl-C. Standard library only.

import datetime
import socket
import sys

out_path = sys.argv[1]
port = int(sys.argv[2]) if len(sys.argv) > 2 else 5514

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", port))
print(f"listening on udp/{port}, writing to {out_path}", file=sys.stderr)

with open(out_path, "a", encoding="utf-8") as out:
    try:
        while True:
            data, _ = sock.recvfrom(2048)
            text = data.decode("utf-8", errors="replace").replace("﻿", "").rstrip("\r\n")
            stamp = datetime.datetime.now().isoformat(timespec="milliseconds")
            out.write(f"{stamp}\t{text}\n")
            out.flush()  # a crash or power cut loses at most the current line
    except KeyboardInterrupt:
        pass
