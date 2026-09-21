#include "WiFiManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
static const char* statusName(wl_status_t s) {
  switch (s) {
    case WL_NO_SSID_AVAIL: return "SSID not found";
    case WL_CONNECT_FAILED: return "connect failed (wrong password?)";
    case WL_CONNECTION_LOST: return "connection lost";
    case WL_DISCONNECTED: return "disconnected";
    case WL_IDLE_STATUS: return "idle/timeout";
    default: return "unknown";
  }
}
bool WiFiManager::begin(const String& ssid, const String& password, const String& hostname) {
  hostname_ = hostname;
  if (ssid.isEmpty()) { startAp(); return false; }
  WiFi.persistent(false); // we already persist credentials ourselves via ConfigManager/NVS; avoids extra flash wear
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false); // clear any stale connection state without powering off the radio
  delay(300); // let the driver settle before (re)connecting, avoids flaky first attempts
  WiFi.setSleep(false); // modem sleep causes missed beacons/dropped or failed connections on many routers
  WiFi.setHostname(hostname.c_str());
  WiFi.setAutoReconnect(true); // let the driver handle reconnects after a temporary drop, per Espressif recommendation
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN); // scan every channel instead of stopping at the first match
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL); // prefer the strongest AP if the SSID has multiple access points
  WiFi.begin(ssid.c_str(), password.c_str());
  startedAt_ = millis();
  wl_status_t status;
  while ((status = WiFi.status()) != WL_CONNECTED && millis() - startedAt_ < 15000) delay(100);
  if (status != WL_CONNECTED) {
    Serial.printf("[RedButton] WiFi STA connect failed: %s, starting setup AP\n", statusName(status));
    startAp(); return false;
  }
  MDNS.begin(hostname.c_str()); MDNS.addService("http", "tcp", 80); apMode_ = false; wasConnected_ = true; return true;
}
void WiFiManager::startAp() {
  WiFi.disconnect(true); WiFi.mode(WIFI_AP_STA);
  uint64_t chip = ESP.getEfuseMac(); char ssid[24]; snprintf(ssid, sizeof(ssid), "RedButton-%04X", (uint16_t)chip);
  WiFi.softAP(ssid); apMode_ = true; startedAt_ = millis();
  dnsServer_.start(53, "*", WiFi.softAPIP()); // redirect every DNS query so phones show the captive-portal login prompt
}
void WiFiManager::loop() {
  if (apMode_) { dnsServer_.processNextRequest(); return; } // deliberately retain the setup AP until credentials are saved and rebooted
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected_) { Serial.printf("[RedButton] WiFi %s\n", connected ? "reconnected" : "disconnected, auto-reconnecting..."); wasConnected_ = connected; }
}
String WiFiManager::ip() const { return (apMode_ ? WiFi.softAPIP() : WiFi.localIP()).toString(); }
