#include "ConfigManager.h"

// Only the GPIOs actually broken out as D0-D10 on the Seeed XIAO ESP32-S3 header.
static const int8_t kAllowedGpio[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 43, 44};

bool ConfigManager::begin() {
  if (!preferences_.begin("redbutton", false)) return false;
  config_.version = preferences_.getUShort("version", AppConfig::CURRENT_VERSION);
  if (config_.version != AppConfig::CURRENT_VERSION) { factoryReset(); return true; }
  config_.deviceName = preferences_.getString("name", "redbutton");
  config_.wifiSsid = preferences_.getString("ssid", "");
  config_.wifiPassword = preferences_.getString("wifiPass", "");
  config_.buttonCount = preferences_.getUChar("btnCount", 1);
  if (config_.buttonCount < 1 || config_.buttonCount > AppConfig::MAX_BUTTONS) config_.buttonCount = 1;
  for (uint8_t i = 0; i < AppConfig::MAX_BUTTONS; i++) {
    const String p = "b" + String(i);
    auto& b = config_.buttons[i];
    b.gpio = preferences_.getChar((p + "gpio").c_str(), i == 0 ? 2 : -1);
    b.actionType = static_cast<ActionType>(preferences_.getUChar((p + "type").c_str(), i == 0 ? 1 : 0));
    b.hidKey = preferences_.getString((p + "key").c_str(), "K");
    b.modifiers = preferences_.getUChar((p + "mods").c_str(), ModCtrl | ModShift);
    b.serialAction = preferences_.getString((p + "event").c_str(), "custom_1");
  }
  return true;
}

bool ConfigManager::save() {
  // Do not short-circuit: Preferences reports zero bytes for a valid empty string.
  preferences_.putUShort("version", AppConfig::CURRENT_VERSION);
  preferences_.putString("name", config_.deviceName);
  preferences_.putString("ssid", config_.wifiSsid);
  preferences_.putString("wifiPass", config_.wifiPassword);
  preferences_.putUChar("btnCount", config_.buttonCount);
  for (uint8_t i = 0; i < AppConfig::MAX_BUTTONS; i++) {
    const String p = "b" + String(i);
    const auto& b = config_.buttons[i];
    preferences_.putChar((p + "gpio").c_str(), b.gpio);
    preferences_.putUChar((p + "type").c_str(), static_cast<uint8_t>(b.actionType));
    preferences_.putString((p + "key").c_str(), b.hidKey);
    preferences_.putUChar((p + "mods").c_str(), b.modifiers);
    preferences_.putString((p + "event").c_str(), b.serialAction);
  }
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
bool ConfigManager::validGpio(int gpio) {
  for (int8_t allowed : kAllowedGpio) if (allowed == gpio) return true;
  return false;
}
