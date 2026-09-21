#pragma once
#include <Arduino.h>
#include <functional>
class ButtonManager {
 public:
  using Callback = std::function<void()>;
  void begin(uint8_t pin, Callback callback, uint32_t debounceMs = 35);
  void loop();
 private:
  uint8_t pin_ = 0; uint32_t debounceMs_ = 35; uint32_t changedAt_ = 0;
  bool raw_ = HIGH, stable_ = HIGH; bool active_ = false; Callback callback_;
};

