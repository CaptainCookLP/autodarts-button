# Red Button for XIAO ESP32-S3

A maintainable, locally managed programmable button. The firmware exposes native USB HID keyboard **and** USB CDC serial concurrently, a responsive no-CDN control panel, Wi-Fi provisioning, NVS configuration and dual-slot browser OTA. A defensive Linux daemon and a Chromium extension example complete the project.

## 1. Project overview

| Directory | Purpose |
|---|---|
| `src/`, `include/` | Componentized Arduino firmware |
| `data/` | LittleFS web UI, uploaded separately from firmware |
| `linux/` | pyserial daemon, systemd unit, udev rule |
| `browser-extension/` | Manifest V3 HID example and native host skeleton |
| `docs/` | Architecture, extension and rollback decisions |

Firmware version is defined centrally as `REDBUTTON_VERSION` in `platformio.ini`. Configuration schema version 1 is stored in the `redbutton` NVS namespace. Unknown schema versions are reset safely; add explicit migrations before incrementing it.

## 2. Hardware and wiring

Target: Seeed Studio XIAO ESP32-S3 (native USB-C, Wi-Fi; battery optional). The default input is board pin `D1`. The external button must be a dry, normally-open contact—do not inject voltage.

```text
               XIAO ESP32-S3
             +---------------+
Button NO ---| D1 / GPIO     |  configured INPUT_PULLUP
Button COM --| GND           |
             +-------+-------+
                     |
                  USB-C ---- Linux / browser host
```

The stable LOW transition is accepted after 35 ms and fires once. Holding the button cannot retrigger it; it must be released and pressed again.

## 3. PlatformIO setup, build and flash

Install VS Code + PlatformIO, or [PlatformIO Core](https://platformio.org/install/cli). Then:

```bash
pio run
pio run -t upload
pio run -t uploadfs       # required once and whenever data/ changes
pio device monitor -b 115200
```

The pinned Espressif32 platform uses Arduino-ESP32. `ARDUINO_USB_MODE=0` selects the ESP32-S3 native USB OTG peripheral and `ARDUINO_USB_CDC_ON_BOOT=1` enables CDC alongside `USBHIDKeyboard`. If upload becomes difficult, hold BOOT, tap RESET, release BOOT, and select the new serial port.

## 4. First Wi-Fi setup

Without stored credentials—or after a 15-second failed connection—the device creates `RedButton-XXXX`. Join it and open **http://192.168.4.1**. Scan, choose an SSID, enter its password, save and reboot. In the LAN use **http://redbutton.local** or the IP shown by your router. Credentials are held in NVS, omitted from config export and never logged.

To recover from bad credentials, let connection timeout and use the setup AP. Factory reset is available under System and requires typing `RESET`; it erases Wi-Fi and action settings.

## 5. Web interface and REST API

Status displays device/firmware, uptime, SSID, IP, RSSI, USB CDC-open state, current action and last event. Button settings offer Disabled, HID shortcut and serial event. The test button uses exactly the same action dispatcher as the physical input.

| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/status` | live state |
| GET/POST | `/api/config` | export/update non-secret action config |
| GET | `/api/config/export` | download non-secret JSON |
| GET | `/api/wifi/scan` | up to 30 networks |
| POST | `/api/wifi` | store SSID/password; reboot required |
| POST | `/api/action/test` | run configured action |
| POST | `/api/system/reboot` | reboot |
| POST | `/api/system/factory-reset` | requires `X-Confirm-Reset: RESET` |
| POST | `/api/ota` | multipart `.bin` upload |

Inputs have length, range and character validation. This LAN-oriented release has no login: the server boundary is prepared for authentication middleware, but the device **must not be port-forwarded** to the Internet.

## 6. HID configuration

Select modifiers and a key (letters, digits, navigation, F1–F12). The Arduino USB keyboard API used here exposes F1–F12; F13–F24 are deliberately not advertised because portable constants are unavailable in the pinned core. HID sends modifiers, presses the key briefly, then releases all keys. On macOS, GUI maps to Command; elsewhere it maps to Windows/Super.

## 7. USB serial protocol

CDC emits one UTF-8 JSON object per line, for example:

```json
{"event":"button","action":"presentation_next"}
```

Action IDs contain only ASCII letters, digits, `_` or `-` and are at most 64 characters. Firmware never accepts or sends shell commands. CDC debug output is intentionally minimal; consumers must ignore JSON objects whose `event` is not `button`.

## 8. OTA update

Build with `pio run`; upload `.pio/build/seeed_xiao_esp32s3/firmware.bin` in Firmware Update. The handler rejects non-`.bin` names and checks all `Update` writes. `partitions.csv` contains NVS, OTA metadata, two 1.75 MiB application slots and LittleFS. Do not upload `littlefs.bin` as firmware.

Dual slots keep an interrupted upload from replacing the running image, but application-confirmed rollback is not enabled by Arduino `Update`; see [the architecture notes](docs/ARCHITECTURE.md). Perform initial and recovery updates over USB and keep a known-good binary.

## 9. Linux daemon installation

```bash
sudo useradd --system --no-create-home --groups dialout redbutton
sudo mkdir -p /opt/redbutton /etc/redbutton
sudo cp linux/redbutton-daemon/redbutton_daemon.py /opt/redbutton/
sudo cp linux/redbutton-daemon/actions.example.json /etc/redbutton/actions.json
sudo python3 -m venv /opt/redbutton/venv
sudo /opt/redbutton/venv/bin/pip install -r linux/redbutton-daemon/requirements.txt
sudo cp linux/systemd/redbutton-daemon.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now redbutton-daemon
journalctl -u redbutton-daemon -f
```

Edit `/etc/redbutton/actions.json`. Each value is an argv array and is executed with `shell=False`; only exact IDs in that local allowlist run. Scripts should be root-owned and not writable by the `redbutton` user. The daemon reconnects every two seconds after unplugging.

## 10. udev stable device name

```bash
sudo cp linux/udev/99-redbutton.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
udevadm info --attribute-walk /dev/ttyACM0
```

The rule matches Espressif's default VID `303a` plus product string and creates `/dev/redbutton`. Confirm actual values with `udevadm`; customized Arduino USB VID/PID build flags or core updates require adjusting the rule. `USB.productName()` sets the product. The S3 exposes a chip-derived USB serial by default; add a verified `USB.serialNumber(...)` before `USB.begin()` if deployments require a fixed custom serial.

## 11. Chromium extension

### Variant A: HID (recommended)

Open `chrome://extensions`, enable Developer mode, choose **Load unpacked**, and select `browser-extension/`. Configure firmware to Ctrl+Shift+K. Chrome's `commands` API receives it without a bridge; the sample reloads the active tab. Change `background.js` to the desired browser-only behavior. Shortcut conflicts can be resolved at `chrome://extensions/shortcuts`.

### Variant B: native messaging

The intended flow is ESP CDC → allowlisting daemon → separately supervised native bridge → extension. `native-host.py` demonstrates Chrome's length-prefixed output and a second allowlist; `org.redbutton.native.json.example` documents host registration. Production integration must add the extension ID, `nativeMessaging` permission, `connectNative()` and IPC from the long-running daemon. See `docs/ARCHITECTURE.md`; do not weaken the daemon into executing browser-supplied commands.

## 12. Configuration backup/import

Export downloads action and device settings but never Wi-Fi secrets. Import accepts that JSON through the System panel (or it can be posted to `/api/config`). Wi-Fi is intentionally restored separately.

## 13. Troubleshooting

* **No web UI:** upload LittleFS with `pio run -t uploadfs`; try the IP if `.local` is unavailable.
* **No setup AP:** wait at least 15 seconds; power-cycle. Its default address is `192.168.4.1`.
* **HID absent:** use the native USB-C data connection and ensure the build flags were not overridden.
* **Serial absent:** inspect `dmesg`, `pio device list`, and udev attributes; some charge-only cables have no data lines.
* **Repeated presses:** verify NO-to-GND wiring and avoid long unshielded cable runs; increase `debounceMs` if necessary.
* **OTA rejects image:** use the application `firmware.bin`, not filesystem or factory images; recover through USB.
* **Daemon does nothing:** check `/dev/redbutton`, membership in `dialout`, JSON syntax and `journalctl`.

## 14. Hardware validation checklist

Software builds/tests cannot prove electrical behavior. On the target, verify native HID+CDC enumeration on Linux/macOS/Windows; one event per press including contact bounce and long holds; reconnection and AP fallback; mDNS; all desired host keyboard layouts; battery behavior; OTA success, interrupted-upload recovery and USB recovery; daemon unplug/replug; and the actual VID/PID/product/serial values.
