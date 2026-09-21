#include <Arduino.h>
#include "ActionManager.h"
#include "ButtonManager.h"
#include "ConfigManager.h"
#include "USBManager.h"
#include "UpdateManager.h"
#include "WebServerManager.h"
#include "WiFiManager.h"

ConfigManager config;
USBManager usb;
WiFiManager wifi;
ButtonManager buttons[AppConfig::MAX_BUTTONS];
ActionManager action(config.get(), usb);
UpdateManager updater(REDBUTTON_VERSION);
WebServerManager web(config, wifi, action, usb, updater);

void setup() {
  Serial.begin(115200);
  delay(1500); // give the native USB CDC host time to enumerate so early boot logs aren't lost
  Serial.println("[RedButton] boot " REDBUTTON_VERSION);

  config.begin();
  Serial.println("[RedButton] config loaded");

  // Start WiFi/AP before USB HID setup so a USB init problem can't block the setup hotspot.
  wifi.begin(config.get().wifiSsid, config.get().wifiPassword, config.get().deviceName);
  Serial.printf("[RedButton] wifi ready mode=%s ip=%s\n", wifi.isAp() ? "AP" : "STA", wifi.ip().c_str());

  usb.begin();
  Serial.println("[RedButton] usb ready");

  const auto& cfg = config.get();
  for (uint8_t i = 0; i < cfg.buttonCount; i++) {
    if (cfg.buttons[i].gpio < 0) continue;
    buttons[i].begin(cfg.buttons[i].gpio, [i] { action.trigger(i); });
  }
  Serial.printf("[RedButton] %d button(s) ready\n", cfg.buttonCount);

  web.begin();
  Serial.println("[RedButton] web server ready");
}
void loop() {
  for (uint8_t i = 0; i < config.get().buttonCount; i++) buttons[i].loop();
  wifi.loop(); web.loop(); updater.loop();
  delay(1);
}

