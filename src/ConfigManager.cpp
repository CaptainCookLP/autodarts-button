#include "ConfigManager.h"

bool ConfigManager::begin() {
  if (!preferences_.begin("redbutton", false)) return false;
  config_.version = preferences_.getUShort("version", AppConfig::CURRENT_VERSION);
  if (config_.version != AppConfig::CURRENT_VERSION) { factoryReset(); return true; }
  config_.deviceName = preferences_.getString("name", "redbutton");
  config_.wifiSsid = preferences_.getString("ssid", "");
  config_.wifiPassword = preferences_.getString("wifiPass", "");
  config_.actionType = static_cast<ActionType>(preferences_.getUChar("action", 1));
  config_.hidKey = preferences_.getString("key", "K");
  config_.modifiers = preferences_.getUChar("mods", ModCtrl | ModShift);
  config_.serialAction = preferences_.getString("event", "custom_1");
  return true;
}

bool ConfigManager::save() {
  // Do not short-circuit: Preferences reports zero bytes for a valid empty string.
  preferences_.putUShort("version", AppConfig::CURRENT_VERSION);
  preferences_.putString("name", config_.deviceName);
  preferences_.putString("ssid", config_.wifiSsid);
  preferences_.putString("wifiPass", config_.wifiPassword);
  preferences_.putUChar("action", static_cast<uint8_t>(config_.actionType));
  preferences_.putString("key", config_.hidKey);
  preferences_.putUChar("mods", config_.modifiers);
  preferences_.putString("event", config_.serialAction);
  return preferences_.getUShort("version", 0) == AppConfig::CURRENT_VERSION &&
         preferences_.getString("name", "") == config_.deviceName;
}

bool ConfigManager::factoryReset() { preferences_.clear(); config_ = AppConfig{}; return save(); }
bool ConfigManager::validName(const String& v) {
  if (v.length() < 1 || v.length() > 32) return false;
  for (char c : v) if (!(isalnum(c) || c == '-')) return false;
  return true;
}
bool ConfigManager::validActionId(const String& v) {
  if (v.length() < 1 || v.length() > 64) return false;
  for (char c : v) if (!(isalnum(c) || c == '_' || c == '-')) return false;
  return true;
}
