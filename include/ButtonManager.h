#pragma once
#include <Arduino.h>
#include <functional>
class ButtonManager {
 public:
  using Callback = std::function<void()>;
  ButtonManager(uint8_t pin, uint32_t debounceMs = 35) : pin_(pin), debounceMs_(debounceMs) {}
  void begin(Callback callback);
  void loop();
 private:
  uint8_t pin_; uint32_t debounceMs_; uint32_t changedAt_ = 0;
  bool raw_ = HIGH, stable_ = HIGH; Callback callback_;
};

