import importlib.util, json
from pathlib import Path
spec=importlib.util.spec_from_file_location("daemon",Path(__file__).with_name("redbutton_daemon.py")); d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
def test_allowed_action_uses_argv_without_shell():
    calls=[]
    assert d.process_line('{"event":"button","action":"mute"}',{"mute":["/bin/true"]},lambda *a,**k:calls.append((a,k)))
    assert calls==[((["/bin/true"],),{"shell":False,"close_fds":True})]
def test_rejects_unknown_and_malformed():
    called=[]
    assert not d.process_line('{"event":"button","action":"evil"}',{},lambda *a,**k:called.append(1))
    assert not d.process_line('nope',{},lambda *a,**k:called.append(1)); assert not called

