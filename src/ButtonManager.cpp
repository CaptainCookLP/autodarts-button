#include "ButtonManager.h"
void ButtonManager::begin(uint8_t pin, Callback cb, uint32_t debounceMs) {
  pin_ = pin; debounceMs_ = debounceMs; callback_ = cb; active_ = true;
  pinMode(pin_, INPUT_PULLUP); raw_ = stable_ = digitalRead(pin_);
}
void ButtonManager::loop() {
  if (!active_) return;
  const bool value = digitalRead(pin_);
  if (value != raw_) { raw_ = value; changedAt_ = millis(); }
  if (value != stable_ && millis() - changedAt_ >= debounceMs_) {
    stable_ = value;
    if (stable_ == LOW && callback_) callback_();
  }
}

