#pragma once
#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
class USBManager {
 public:
  void begin();
  bool sendShortcut(const String& key, uint8_t modifiers);
  void sendEvent(const String& actionId);
  bool connected() const { return USBSerial; }
 private:
  USBHIDKeyboard keyboard_;
  uint8_t keyCode(const String& key) const;
};

