#pragma once
#include <Preferences.h>
#include "AppConfig.h"
class ConfigManager {
 public:
  bool begin();
  const AppConfig& get() const { return config_; }
  AppConfig& edit() { return config_; }
  bool save();
  bool factoryReset();
  static bool validName(const String& value);
  static bool validActionId(const String& value);
  static bool validGpio(int gpio);
 private:
  Preferences preferences_;
  AppConfig config_;
};

