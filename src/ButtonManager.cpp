#include "ButtonManager.h"
void ButtonManager::begin(Callback cb) { callback_ = cb; pinMode(pin_, INPUT_PULLUP); raw_ = stable_ = digitalRead(pin_); }
void ButtonManager::loop() {
  const bool value = digitalRead(pin_);
  if (value != raw_) { raw_ = value; changedAt_ = millis(); }
  if (value != stable_ && millis() - changedAt_ >= debounceMs_) {
    stable_ = value;
    if (stable_ == LOW && callback_) callback_();
  }
}

