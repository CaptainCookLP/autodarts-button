#include "WebServerManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>

void WebServerManager::json(int status, const String& body) { server_.send(status, "application/json", body); }
bool WebServerManager::parseBody(JsonDocument& doc) {
  if (!server_.hasArg("plain") || server_.arg("plain").length() > 4096) { json(400, "{\"error\":\"invalid body\"}"); return false; }
  if (deserializeJson(doc, server_.arg("plain"))) { json(400, "{\"error\":\"invalid JSON\"}"); return false; } return true;
}
void WebServerManager::begin() {
  const char* headers[] = {"X-Confirm-Reset"}; server_.collectHeaders(headers, 1);
  // LittleFS.begin() defaults to a partition labeled "spiffs"; ours is named "littlefs" in partitions.csv.
  const bool mounted = LittleFS.begin(false, "/littlefs", 10, "littlefs");
  if (!mounted) { Serial.println("[RedButton] LittleFS mount failed, formatting..."); LittleFS.begin(true, "/littlefs", 10, "littlefs"); }
  Serial.printf("[RedButton] LittleFS ready, index.html present=%d\n", LittleFS.exists("/index.html"));
  routes(); server_.begin();
}
void WebServerManager::routes() {
  auto sendIndex = [this]{
    File f = LittleFS.open("/index.html");
    if (!f) { json(500, "{\"error\":\"filesystem not flashed, run 'pio run -t uploadfs'\"}"); return; }
    server_.streamFile(f, "text/html");
  };
  server_.on("/", HTTP_GET, sendIndex);
  server_.serveStatic("/app.js", LittleFS, "/app.js"); server_.serveStatic("/style.css", LittleFS, "/style.css");

  // Captive-portal probe URLs used by phones/laptops to detect the setup hotspot.
  server_.on("/generate_204", HTTP_GET, sendIndex);
  server_.on("/hotspot-detect.html", HTTP_GET, sendIndex);
  server_.on("/connecttest.txt", HTTP_GET, sendIndex);
  server_.on("/ncsi.txt", HTTP_GET, sendIndex);

  server_.on("/api/status", HTTP_GET, [this]{
    JsonDocument d; const auto& c = config_.get();
    d["deviceName"] = c.deviceName; d["firmware"] = REDBUTTON_VERSION; d["uptimeSeconds"] = millis() / 1000;
    d["ssid"] = wifi_.isAp() ? "setup AP" : WiFi.SSID(); d["ip"] = wifi_.ip(); d["rssi"] = wifi_.isAp() ? 0 : WiFi.RSSI();
    d["usbConnected"] = usb_.connected(); d["buttonCount"] = c.buttonCount; d["lastEvent"] = action_.lastEvent();
    String out; serializeJson(d, out); json(200, out);
  });

  server_.on("/api/config", HTTP_GET, [this]{
    JsonDocument d; const auto& c = config_.get();
    d["version"] = c.version; d["deviceName"] = c.deviceName;
    auto arr = d["buttons"].to<JsonArray>();
    for (uint8_t i = 0; i < c.buttonCount; i++) {
      const auto& b = c.buttons[i]; auto o = arr.add<JsonObject>();
      o["gpio"] = b.gpio; o["actionType"] = (int)b.actionType; o["hidKey"] = b.hidKey; o["modifiers"] = b.modifiers; o["serialAction"] = b.serialAction;
    }
    String out; serializeJson(d, out); json(200, out);
  });
  server_.on("/api/config", HTTP_POST, [this]{
    JsonDocument d; if (!parseBody(d)) return;
    auto& c = config_.edit();
    String n = d["deviceName"] | c.deviceName;
    if (!ConfigManager::validName(n)) { json(422, "{\"error\":\"invalid device name\"}"); return; }
    JsonArray arr = d["buttons"].as<JsonArray>();
    if (arr.isNull() || arr.size() < 1 || arr.size() > AppConfig::MAX_BUTTONS) { json(422, "{\"error\":\"invalid button count\"}"); return; }
    ButtonConfig parsed[AppConfig::MAX_BUTTONS]; uint8_t count = 0;
    for (JsonObject b : arr) {
      int gpio = b["gpio"] | -1; int type = b["actionType"] | 0; String key = b["hidKey"] | "K";
      int mods = b["modifiers"] | 0; String event = b["serialAction"] | "custom_1";
      if (!ConfigManager::validGpio(gpio) || type < 0 || type > 2 || key.length() < 1 || key.length() > 12 || mods < 0 || mods > 15 || !ConfigManager::validActionId(event)) {
        json(422, "{\"error\":\"invalid button configuration\"}"); return;
      }
      for (uint8_t i = 0; i < count; i++) if (parsed[i].gpio == gpio) { json(422, "{\"error\":\"duplicate GPIO\"}"); return; }
      parsed[count].gpio = gpio; parsed[count].actionType = (ActionType)type; parsed[count].hidKey = key; parsed[count].modifiers = mods; parsed[count].serialAction = event;
      count++;
    }
    c.deviceName = n; c.buttonCount = count;
    for (uint8_t i = 0; i < count; i++) c.buttons[i] = parsed[i];
    const bool saved = config_.save();
    json(saved ? 200 : 500, saved ? "{\"ok\":true,\"rebootRequired\":true}" : "{\"error\":\"NVS write failed\"}");
  });

  server_.on("/api/wifi/scan", HTTP_GET, [this]{
    // Blocking scanNetworks() can take many seconds, especially with the setup AP active concurrently;
    // use the async API and let the client poll instead of stalling the HTTP request.
    int16_t n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) { json(202, "{\"status\":\"scanning\"}"); return; }
    if (n == WIFI_SCAN_FAILED) { WiFi.scanNetworks(true, false, false, 120); json(202, "{\"status\":\"scanning\"}"); return; }
    JsonDocument d; auto a = d.to<JsonArray>();
    for (int i = 0; i < n && i < 30; i++) { auto o = a.add<JsonObject>(); o["ssid"] = WiFi.SSID(i); o["rssi"] = WiFi.RSSI(i); o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN; }
    String out; serializeJson(d, out); json(200, out); WiFi.scanDelete();
  });
  server_.on("/api/wifi", HTTP_POST, [this]{JsonDocument d;if(!parseBody(d))return;String s=d["ssid"]|"",p=d["password"]|"";
    const bool forget = s.isEmpty() && p.isEmpty();
    if(!forget && (s.length()<1||s.length()>32||p.length()>63)){json(422,"{\"error\":\"invalid credentials\"}");return;}
    config_.edit().wifiSsid=s;config_.edit().wifiPassword=p;json(config_.save()?200:500,"{\"ok\":true,\"rebootRequired\":true}");});

  server_.on("/api/action/test", HTTP_POST, [this]{
    int index = 0;
    if (server_.hasArg("plain")) { JsonDocument d; if (!deserializeJson(d, server_.arg("plain"))) index = d["index"] | 0; }
    if (index < 0 || index >= config_.get().buttonCount) { json(422, "{\"error\":\"invalid button index\"}"); return; }
    json(action_.trigger(index) ? 200 : 409, "{\"ok\":true}");
  });

  server_.on("/api/system/reboot", HTTP_POST, [this]{json(202,"{\"ok\":true}");delay(200);ESP.restart();});
  server_.on("/api/system/factory-reset", HTTP_POST, [this]{if(server_.header("X-Confirm-Reset")!="RESET"){json(400,"{\"error\":\"confirmation required\"}");return;}config_.factoryReset();json(202,"{\"ok\":true}");delay(200);ESP.restart();});
  server_.on("/api/config/export", HTTP_GET, [this]{
    JsonDocument d; const auto& c = config_.get();
    d["version"] = c.version; d["deviceName"] = c.deviceName;
    auto arr = d["buttons"].to<JsonArray>();
    for (uint8_t i = 0; i < c.buttonCount; i++) {
      const auto& b = c.buttons[i]; auto o = arr.add<JsonObject>();
      o["gpio"] = b.gpio; o["actionType"] = (int)b.actionType; o["hidKey"] = b.hidKey; o["modifiers"] = b.modifiers; o["serialAction"] = b.serialAction;
    }
    String out; serializeJsonPretty(d, out); server_.sendHeader("Content-Disposition", "attachment; filename=redbutton-config.json"); json(200, out);
  });

  server_.on("/api/ota", HTTP_POST, [this]{
    bool ok=!Update.hasError();
    if(!ok) Serial.printf("[RedButton] OTA failed: %s\n", Update.errorString());
    json(ok?200:500,ok?"{\"ok\":true,\"rebooting\":true}":"{\"error\":\"update failed\"}");
    if(ok){delay(300);ESP.restart();}
  }, [this]{
    HTTPUpload& u=server_.upload();
    if(u.status==UPLOAD_FILE_START){
      if(!u.filename.endsWith(".bin")){Update.abort();return;}
      if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH)) Serial.printf("[RedButton] OTA begin failed: %s\n", Update.errorString());
    } else if(u.status==UPLOAD_FILE_WRITE&&!Update.hasError()) Update.write(u.buf,u.currentSize);
    else if(u.status==UPLOAD_FILE_END&&!Update.hasError()) Update.end(true);
  });

  server_.on("/api/update/check", HTTP_POST, [this]{
    const FirmwareCheckResult r = updater_.checkLatest();
    JsonDocument d; d["ok"]=r.ok; d["available"]=r.available; d["current"]=r.currentVersion; d["latest"]=r.latestVersion; d["size"]=(uint32_t)r.size; d["error"]=r.error;
    String out; serializeJson(d,out); json(r.ok?200:500,out);
  });
  server_.on("/api/update/install", HTTP_POST, [this]{
    if (!updater_.requestInstall()) { json(409, "{\"ok\":false,\"error\":\"no update available or already running\"}"); return; }
    json(202, "{\"ok\":true}");
  });
  server_.on("/api/update/status", HTTP_GET, [this]{
    JsonDocument d; d["state"]=updater_.state(); d["message"]=updater_.message(); d["progress"]=updater_.progress();
    String out; serializeJson(d,out); json(200,out);
  });

  server_.onNotFound([this]{
    if (wifi_.isAp()) { server_.sendHeader("Location", "http://192.168.4.1/", true); server_.send(302, "text/plain", ""); return; }
    json(404, "{\"error\":\"not found\"}");
  });
}

