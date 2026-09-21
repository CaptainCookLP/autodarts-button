#pragma once
#include <Arduino.h>
class WiFiManager {
 public:
  bool begin(const String& ssid, const String& password, const String& hostname);
  void loop();
  bool isAp() const { return apMode_; }
  String ip() const;
 private:
  void startAp();
  bool apMode_ = false; uint32_t startedAt_ = 0; uint32_t lastReconnectAttempt_ = 0; String hostname_;
};

