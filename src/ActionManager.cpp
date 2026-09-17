#include "ActionManager.h"
bool ActionManager::trigger() {
  bool ok = true;
  switch (config_.actionType) {
    case ActionType::Disabled: ok = false; break;
    case ActionType::Hid: ok = usb_.sendShortcut(config_.hidKey, config_.modifiers); break;
    case ActionType::SerialEvent: usb_.sendEvent(config_.serialAction); break;
  }
  lastEvent_ = String(millis()) + " ms: " + name(config_.actionType) + (ok ? "" : " (ignored)"); return ok;
}
const char* ActionManager::name(ActionType t) { if (t == ActionType::Hid) return "hid"; if (t == ActionType::SerialEvent) return "serial"; return "disabled"; }
