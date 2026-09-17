#pragma once
#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
class USBManager {
 public:
  void begin();
  bool sendShortcut(const String& key, uint8_t modifiers);
  void sendEvent(const String& actionId);
  // With ARDUINO_USB_CDC_ON_BOOT the core exposes native TinyUSB CDC as Serial.
  bool connected() const { return static_cast<bool>(Serial); }
 private:
  USBHIDKeyboard keyboard_;
  uint8_t keyCode(const String& key) const;
};
