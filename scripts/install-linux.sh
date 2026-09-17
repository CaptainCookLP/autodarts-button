#!/usr/bin/env bash
set -euo pipefail

[[ ${EUID:-$(id -u)} -eq 0 ]] || { echo "Bitte mit sudo ausführen." >&2; exit 1; }
root="$(cd "$(dirname "$0")/.." && pwd)"
id redbutton >/dev/null 2>&1 || useradd --system --no-create-home --groups dialout redbutton
install -d -m 0755 /opt/redbutton /etc/redbutton
install -m 0755 "$root/linux/redbutton-daemon/redbutton_daemon.py" /opt/redbutton/
[[ -e /etc/redbutton/actions.json ]] || install -m 0640 -o root -g redbutton "$root/linux/redbutton-daemon/actions.example.json" /etc/redbutton/actions.json
python3 -m venv /opt/redbutton/venv
/opt/redbutton/venv/bin/pip install --disable-pip-version-check -r "$root/linux/redbutton-daemon/requirements.txt"
install -m 0644 "$root/linux/systemd/redbutton-daemon.service" /etc/systemd/system/
install -m 0644 "$root/linux/udev/99-redbutton.rules" /etc/udev/rules.d/
systemctl daemon-reload
udevadm control --reload-rules
systemctl enable --now redbutton-daemon
echo "Installiert. Allowlist: /etc/redbutton/actions.json"

