#!/usr/bin/env python3
"""Minimal native-messaging host: forwards validated daemon messages on stdin."""
import json,struct,sys
ALLOWED={"presentation_next","mute","custom_1"}
for line in sys.stdin:
    try:
        msg=json.loads(line); action=msg.get("action")
        if action not in ALLOWED: continue
        payload=json.dumps({"event":"redbutton","action":action}).encode()
        sys.stdout.buffer.write(struct.pack("=I",len(payload))+payload);sys.stdout.buffer.flush()
    except (ValueError,TypeError): pass

