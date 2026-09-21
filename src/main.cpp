#include <Arduino.h>
#include "ActionManager.h"
#include "ButtonManager.h"
#include "ConfigManager.h"
#include "USBManager.h"
#include "WebServerManager.h"
#include "WiFiManager.h"

ConfigManager config;
USBManager usb;
WiFiManager wifi;
ButtonManager button(D1);
ActionManager action(config.get(), usb);
WebServerManager web(config, wifi, action, usb);

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

  button.begin([] { action.trigger(); });
  web.begin();
  Serial.println("[RedButton] web server ready");
}
void loop() {
  button.loop(); wifi.loop(); web.loop();
  static uint32_t lastBeat = 0;
  if (millis() - lastBeat > 5000) { lastBeat = millis(); Serial.println("[RedButton] alive"); } // heartbeat to confirm the firmware is not stuck
  delay(1);
}
