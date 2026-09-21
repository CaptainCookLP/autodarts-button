#include "WebServerManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>

void WebServerManager::json(int status, const String& body) { server_.send(status, "application/json", body); }
bool WebServerManager::parseBody(JsonDocument& doc) {
  if (!server_.hasArg("plain") || server_.arg("plain").length() > 2048) { json(400, "{\"error\":\"invalid body\"}"); return false; }
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
  server_.on("/", HTTP_GET, [this]{
    File f = LittleFS.open("/index.html");
    if (!f) { json(500, "{\"error\":\"filesystem not flashed, run 'pio run -t uploadfs'\"}"); return; }
    server_.streamFile(f, "text/html");
  });
  server_.serveStatic("/app.js", LittleFS, "/app.js"); server_.serveStatic("/style.css", LittleFS, "/style.css");
  server_.on("/api/status", HTTP_GET, [this]{
    JsonDocument d; d["deviceName"]=config_.get().deviceName; d["firmware"]=REDBUTTON_VERSION; d["uptimeSeconds"]=millis()/1000;
    d["ssid"]=wifi_.isAp()?"setup AP":WiFi.SSID(); d["ip"]=wifi_.ip(); d["rssi"]=wifi_.isAp()?0:WiFi.RSSI(); d["usbConnected"]=usb_.connected();
    d["lastEvent"]=action_.lastEvent(); d["action"]=ActionManager::name(config_.get().actionType); String out; serializeJson(d,out); json(200,out);
  });
  server_.on("/api/config", HTTP_GET, [this]{ JsonDocument d; const auto& c=config_.get(); d["version"]=c.version; d["deviceName"]=c.deviceName; d["actionType"]=(int)c.actionType; d["hidKey"]=c.hidKey; d["modifiers"]=c.modifiers; d["serialAction"]=c.serialAction; String out; serializeJson(d,out); json(200,out); });
  server_.on("/api/config", HTTP_POST, [this]{ JsonDocument d; if(!parseBody(d))return; auto& c=config_.edit(); String n=d["deviceName"]|c.deviceName; String k=d["hidKey"]|c.hidKey; String id=d["serialAction"]|c.serialAction; int type=d["actionType"]|int(c.actionType); int mods=d["modifiers"]|c.modifiers;
    if(!ConfigManager::validName(n)||!ConfigManager::validActionId(id)||k.length()<1||k.length()>12||type<0||type>2||mods<0||mods>15){json(422,"{\"error\":\"validation failed\"}");return;} c.deviceName=n;c.hidKey=k;c.serialAction=id;c.actionType=(ActionType)type;c.modifiers=mods; const bool saved=config_.save(); json(saved?200:500,saved?"{\"ok\":true}":"{\"error\":\"NVS write failed\"}"); });
  server_.on("/api/wifi/scan", HTTP_GET, [this]{ int n=WiFi.scanNetworks(); JsonDocument d; auto a=d.to<JsonArray>(); for(int i=0;i<n&&i<30;i++){auto o=a.add<JsonObject>();o["ssid"]=WiFi.SSID(i);o["rssi"]=WiFi.RSSI(i);o["secure"]=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN;} String out;serializeJson(d,out);json(200,out);WiFi.scanDelete(); });
  server_.on("/api/wifi", HTTP_POST, [this]{JsonDocument d;if(!parseBody(d))return;String s=d["ssid"]|"",p=d["password"]|"";if(s.length()<1||s.length()>32||p.length()>63){json(422,"{\"error\":\"invalid credentials\"}");return;}config_.edit().wifiSsid=s;config_.edit().wifiPassword=p;json(config_.save()?200:500,"{\"ok\":true,\"rebootRequired\":true}");});
  server_.on("/api/action/test", HTTP_POST, [this]{json(action_.trigger()?200:409,"{\"ok\":true}");});
  server_.on("/api/system/reboot", HTTP_POST, [this]{json(202,"{\"ok\":true}");delay(200);ESP.restart();});
  server_.on("/api/system/factory-reset", HTTP_POST, [this]{if(server_.header("X-Confirm-Reset")!="RESET"){json(400,"{\"error\":\"confirmation required\"}");return;}config_.factoryReset();json(202,"{\"ok\":true}");delay(200);ESP.restart();});
  server_.on("/api/config/export", HTTP_GET, [this]{JsonDocument d;const auto&c=config_.get();d["version"]=c.version;d["deviceName"]=c.deviceName;d["actionType"]=(int)c.actionType;d["hidKey"]=c.hidKey;d["modifiers"]=c.modifiers;d["serialAction"]=c.serialAction;String out;serializeJsonPretty(d,out);server_.sendHeader("Content-Disposition","attachment; filename=redbutton-config.json");json(200,out);});
  server_.on("/api/ota", HTTP_POST, [this]{bool ok=!Update.hasError();json(ok?200:500,ok?"{\"ok\":true,\"rebooting\":true}":"{\"error\":\"update failed\"}");if(ok){delay(300);ESP.restart();}}, [this]{HTTPUpload& u=server_.upload();if(u.status==UPLOAD_FILE_START){if(!u.filename.endsWith(".bin")){Update.abort();return;}Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH);}else if(u.status==UPLOAD_FILE_WRITE&&!Update.hasError())Update.write(u.buf,u.currentSize);else if(u.status==UPLOAD_FILE_END&&!Update.hasError())Update.end(true);});
  server_.onNotFound([this]{json(404,"{\"error\":\"not found\"}");});
}
