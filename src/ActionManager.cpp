#include "ActionManager.h"
bool ActionManager::trigger(uint8_t index) {
  if (index >= config_.buttonCount) return false;
  const auto& btn = config_.buttons[index];
  bool ok = true;
  switch (btn.actionType) {
    case ActionType::Disabled: ok = false; break;
    case ActionType::Hid: ok = usb_.sendShortcut(btn.hidKey, btn.modifiers); break;
    case ActionType::SerialEvent: usb_.sendEvent(btn.serialAction); break;
  }
  lastEvent_ = String(millis()) + " ms: btn" + String(index) + " " + name(btn.actionType) + (ok ? "" : " (ignored)"); return ok;
}
const char* ActionManager::name(ActionType t) { if (t == ActionType::Hid) return "hid"; if (t == ActionType::SerialEvent) return "serial"; return "disabled"; }
