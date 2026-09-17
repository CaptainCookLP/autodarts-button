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
  Serial.begin(115200); config.begin(); usb.begin();
  button.begin([] { action.trigger(); });
  wifi.begin(config.get().wifiSsid, config.get().wifiPassword, config.get().deviceName);
  web.begin();
  Serial.println("[redbutton] ready (credentials are never logged)");
}
void loop() { button.loop(); wifi.loop(); web.loop(); delay(1); }
