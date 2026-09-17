#pragma once
#include "AppConfig.h"
#include "USBManager.h"
class ActionManager {
 public:
  ActionManager(const AppConfig& config, USBManager& usb) : config_(config), usb_(usb) {}
  bool trigger();
  const String& lastEvent() const { return lastEvent_; }
  static const char* name(ActionType type);
 private:
  const AppConfig& config_; USBManager& usb_; String lastEvent_ = "never";
};

