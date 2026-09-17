#include "WiFiManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
bool WiFiManager::begin(const String& ssid, const String& password, const String& hostname) {
  hostname_ = hostname;
  if (ssid.isEmpty()) { startAp(); return false; }
  WiFi.mode(WIFI_STA); WiFi.setHostname(hostname.c_str()); WiFi.begin(ssid.c_str(), password.c_str()); startedAt_ = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt_ < 15000) delay(100);
  if (WiFi.status() != WL_CONNECTED) { startAp(); return false; }
  MDNS.begin(hostname.c_str()); MDNS.addService("http", "tcp", 80); return true;
}
void WiFiManager::startAp() {
  WiFi.disconnect(true); WiFi.mode(WIFI_AP_STA);
  uint64_t chip = ESP.getEfuseMac(); char ssid[24]; snprintf(ssid, sizeof(ssid), "RedButton-%04X", (uint16_t)chip);
  WiFi.softAP(ssid); apMode_ = true; startedAt_ = millis();
}
void WiFiManager::loop() { /* Deliberately retain the setup AP until credentials are saved and rebooted. */ }
String WiFiManager::ip() const { return (apMode_ ? WiFi.softAPIP() : WiFi.localIP()).toString(); }
