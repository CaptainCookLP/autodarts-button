#!/usr/bin/env python3
"""Read Red Button JSON events and execute only exact argv allowlist entries."""
import argparse, json, logging, subprocess, time
from pathlib import Path
import serial

LOG = logging.getLogger("redbutton")
def load_actions(path):
    data=json.loads(Path(path).read_text())
    actions=data.get("actions",{})
    if not isinstance(actions,dict) or any(not isinstance(v,list) or not v or not all(isinstance(x,str) for x in v) for v in actions.values()):
        raise ValueError("every action must be a non-empty argv array")
    return actions
def process_line(line, actions, runner=subprocess.Popen):
    try: message=json.loads(line)
    except (json.JSONDecodeError,UnicodeError): LOG.warning("discarding malformed event"); return False
    action=message.get("action")
    if message.get("event")!="button" or not isinstance(action,str) or action not in actions:
        LOG.warning("discarding unknown event/action %r",action); return False
    runner(actions[action],shell=False,close_fds=True); LOG.info("started allowed action %s",action); return True
def run(device,config):
    actions=load_actions(config)
    while True:
        try:
            LOG.info("connecting to %s",device)
            with serial.Serial(device,115200,timeout=2) as port:
                while True:
                    line=port.readline()
                    if line: process_line(line.decode("utf-8","strict"),actions)
        except (serial.SerialException,OSError,UnicodeError) as exc:
            LOG.warning("device unavailable: %s; retrying",exc); time.sleep(2)
def main():
    p=argparse.ArgumentParser();p.add_argument("--device",default="/dev/redbutton");p.add_argument("--config",default="/etc/redbutton/actions.json");a=p.parse_args()
    logging.basicConfig(level=logging.INFO,format="%(levelname)s %(message)s");run(a.device,a.config)
if __name__=="__main__": main()

