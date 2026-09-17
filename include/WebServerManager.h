#pragma once
#include <WebServer.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "WiFiManager.h"
#include "ActionManager.h"
class WebServerManager {
 public:
  WebServerManager(ConfigManager& c, WiFiManager& w, ActionManager& a, USBManager& u) : config_(c), wifi_(w), action_(a), usb_(u), server_(80) {}
  void begin(); void loop() { server_.handleClient(); }
 private:
  void routes(); void json(int status, const String& body); bool parseBody(JsonDocument& doc);
  ConfigManager& config_; WiFiManager& wifi_; ActionManager& action_; USBManager& usb_; WebServer server_;
};
