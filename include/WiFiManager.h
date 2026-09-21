#pragma once
#include <Arduino.h>
#include <DNSServer.h>
class WiFiManager {
 public:
  bool begin(const String& ssid, const String& password, const String& hostname);
  void loop();
  bool isAp() const { return apMode_; }
  String ip() const;
 private:
  void startAp();
  bool apMode_ = false; bool wasConnected_ = true; uint32_t startedAt_ = 0; String hostname_;
  DNSServer dnsServer_;
};

