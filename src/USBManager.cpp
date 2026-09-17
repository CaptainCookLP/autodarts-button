#include "USBManager.h"
#include "AppConfig.h"
void USBManager::begin() { USB.productName("Red Button"); USB.manufacturerName("RedButton Project"); keyboard_.begin(); USB.begin(); }
uint8_t USBManager::keyCode(const String& value) const {
  String k = value; k.toUpperCase();
  if (k.length() == 1 && ((k[0] >= 'A' && k[0] <= 'Z') || (k[0] >= '0' && k[0] <= '9'))) return k[0];
  if (k == "ENTER") return KEY_RETURN; if (k == "SPACE") return ' '; if (k == "TAB") return KEY_TAB;
  if (k == "ESC") return KEY_ESC; if (k == "UP") return KEY_UP_ARROW; if (k == "DOWN") return KEY_DOWN_ARROW;
  if (k == "LEFT") return KEY_LEFT_ARROW; if (k == "RIGHT") return KEY_RIGHT_ARROW;
  if (k[0] == 'F') { int n = k.substring(1).toInt(); if (n >= 1 && n <= 12) return KEY_F1 + n - 1; }
  return 0;
}
bool USBManager::sendShortcut(const String& key, uint8_t mods) {
  uint8_t code = keyCode(key); if (!code) return false;
  if (mods & ModCtrl) keyboard_.press(KEY_LEFT_CTRL); if (mods & ModShift) keyboard_.press(KEY_LEFT_SHIFT);
  if (mods & ModAlt) keyboard_.press(KEY_LEFT_ALT); if (mods & ModGui) keyboard_.press(KEY_LEFT_GUI);
  keyboard_.press(code); delay(15); keyboard_.releaseAll(); return true;
}
void USBManager::sendEvent(const String& id) { Serial.printf("{\"event\":\"button\",\"action\":\"%s\"}\n", id.c_str()); }
