# Architecture and extension points

`ButtonManager` owns edge debouncing and produces exactly one callback per stable press. `ActionManager` is the dispatch boundary: new HTTP/MQTT actions can be added without coupling them to GPIO or the web API. `ConfigManager` owns versioned NVS data. `USBManager` owns the native composite USB interfaces. `WiFiManager` chooses station or provisioning AP mode, while `WebServerManager` exposes validated JSON resources.

Future multiple buttons, long presses and double clicks should produce a richer input event and remain independent from action implementations. Authentication can be added as middleware in `WebServerManager`; do not expose this device to the public Internet before doing so.

## OTA rollback

The two OTA slots permit safe replacement and bootloader fallback when an image cannot boot. Arduino `Update` does not by itself establish application-level health confirmation. Automatic rollback therefore remains intentionally disabled; add ESP-IDF pending-verify marking plus an early self-test before relying on unattended remote updates. A failed browser upload leaves the active slot unchanged.

## Native messaging

HID mode needs only `chrome.commands`. The native example is a framing adapter: configure the daemon's allowlisted action to launch or feed a supervised bridge, install the host manifest in Chrome's native-messaging host directory, and add `nativeMessaging` plus `connectNative()` in the extension for production. Keep validation on both sides.
