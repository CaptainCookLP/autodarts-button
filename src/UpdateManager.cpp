#include "UpdateManager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <time.h>
#include "mbedtls/sha256.h"

namespace {
constexpr const char* kOwner = "CaptainCookLP";
constexpr const char* kRepo = "autodarts-button";
constexpr const char* kAsset = "firmware.bin";

// DigiCert Global Root G2, valid until 2038; signs GitHub's API and release-asset TLS certificates.
const char kRootCA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

bool parseVersion(String v, int out[4]) {
  v.trim();
  if (v.startsWith("v") || v.startsWith("V")) v.remove(0, 1);
  int cut = v.indexOf('-'); if (cut >= 0) v = v.substring(0, cut);
  cut = v.indexOf('+'); if (cut >= 0) v = v.substring(0, cut);
  for (int i = 0; i < 4; i++) out[i] = 0;
  int part = 0; String cur;
  for (size_t i = 0; i <= v.length(); i++) {
    const bool end = i == v.length();
    const char c = end ? '.' : v[i];
    if (c == '.') {
      if (cur.length() == 0 || part >= 4) return false;
      out[part++] = cur.toInt(); cur = "";
      continue;
    }
    if (c < '0' || c > '9') return false;
    cur += c;
  }
  return part > 0;
}

// Returns >0 if a is newer than b, <0 if older, 0 if equal or unparsable.
int compareVersions(const String& a, const String& b) {
  int pa[4], pb[4];
  if (!parseVersion(a, pa) || !parseVersion(b, pb)) return 0;
  for (int i = 0; i < 4; i++) if (pa[i] != pb[i]) return pa[i] > pb[i] ? 1 : -1;
  return 0;
}

bool ensureClock() {
  if (time(nullptr) > 1700000000) return true; // already synced (~Nov 2023)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  const uint32_t start = millis();
  while (millis() - start < 12000) {
    if (time(nullptr) > 1700000000) return true;
    delay(250);
  }
  return false;
}

String sha256Hex(const uint8_t hash[32]) {
  static const char* hex = "0123456789abcdef";
  String out; out.reserve(64);
  for (int i = 0; i < 32; i++) { out += hex[hash[i] >> 4]; out += hex[hash[i] & 0x0F]; }
  return out;
}
} // namespace

FirmwareCheckResult UpdateManager::checkLatest() {
  FirmwareCheckResult r; r.currentVersion = currentVersion_;
  if (WiFi.status() != WL_CONNECTED) { r.error = "No WiFi connection"; return r; }
  if (!ensureClock()) { r.error = "NTP time sync failed (required for HTTPS)"; return r; }

  const String url = "https://api.github.com/repos/" + String(kOwner) + "/" + String(kRepo) + "/releases/latest";
  WiFiClientSecure client; client.setCACert(kRootCA);
  HTTPClient http; http.setTimeout(15000); http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) { r.error = "Could not start HTTPS connection"; return r; }
  http.addHeader("User-Agent", "autodarts-button");
  http.addHeader("Accept", "application/vnd.github+json");

  const int code = http.GET();
  if (code != HTTP_CODE_OK) { r.error = "GitHub API HTTP error " + String(code); http.end(); return r; }

  JsonDocument filter;
  filter["tag_name"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  filter["assets"][0]["size"] = true;
  filter["assets"][0]["digest"] = true;

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { r.error = "GitHub JSON error: " + String(err.c_str()); return r; }

  const char* tag = doc["tag_name"] | "";
  if (!strlen(tag)) { r.error = "Release has no version tag"; return r; }
  r.latestVersion = tag;
  if (r.latestVersion.startsWith("v") || r.latestVersion.startsWith("V")) r.latestVersion.remove(0, 1);

  r.available = compareVersions(r.latestVersion, currentVersion_) > 0;
  if (!r.available) { r.ok = true; available_ = false; return r; }

  for (JsonObject asset : doc["assets"].as<JsonArray>()) {
    const String name = asset["name"] | "";
    if (name != kAsset) continue;
    downloadUrl_ = String((const char*)(asset["browser_download_url"] | ""));
    r.size = asset["size"] | 0;
    String digest = asset["digest"] | "";
    if (digest.startsWith("sha256:")) digest.remove(0, 7);
    digest.toLowerCase();
    expectedSha256_ = digest;
    break;
  }
  if (downloadUrl_.isEmpty()) { r.error = "Release is missing " + String(kAsset); return r; }
  if (r.size == 0) { r.error = "GitHub reports firmware.bin size 0"; return r; }
  if (expectedSha256_.length() != 64) { r.error = "Release has no valid SHA-256 digest"; return r; }

  expectedSize_ = r.size;
  available_ = true;
  r.ok = true;
  return r;
}

bool UpdateManager::requestInstall() {
  if (pending_ || running_ || !available_) return false;
  pending_ = true; pendingSince_ = millis();
  state_ = "queued"; message_ = "Update starting"; progress_ = 0;
  return true;
}

void UpdateManager::fail(const String& error) {
  running_ = false; pending_ = false; state_ = "error"; message_ = error;
  Serial.printf("[RedButton] Update failed: %s\n", error.c_str());
}

void UpdateManager::loop() {
  if (pending_ && !running_ && millis() - pendingSince_ > 300) { pending_ = false; performUpdate(); }
}

void UpdateManager::performUpdate() {
  running_ = true; state_ = "installing"; message_ = "Downloading firmware"; progress_ = 0;

  if (WiFi.status() != WL_CONNECTED) { fail("WiFi connection lost"); return; }
  if (!ensureClock()) { fail("NTP time sync failed"); return; }

  WiFiClientSecure client; client.setCACert(kRootCA);
  HTTPClient http; http.setTimeout(15000); http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setRedirectLimit(10); http.useHTTP10(true);
  if (!http.begin(client, downloadUrl_)) { fail("Could not start firmware download"); return; }
  http.addHeader("User-Agent", "autodarts-button");
  http.addHeader("Accept-Encoding", "identity");

  const int code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); fail("Firmware download HTTP error " + String(code)); return; }

  const int contentLength = http.getSize();
  if (contentLength > 0 && (size_t)contentLength != expectedSize_) { http.end(); fail("Firmware size mismatch"); return; }

  if (!Update.begin(expectedSize_, U_FLASH)) { http.end(); fail(String("OTA begin failed: ") + Update.errorString()); return; }

  mbedtls_sha256_context sha; mbedtls_sha256_init(&sha); mbedtls_sha256_starts(&sha, 0);
  WiFiClient* stream = http.getStreamPtr();
  uint8_t buffer[4096];
  size_t received = 0; uint32_t lastData = millis();

  while (received < expectedSize_) {
    const size_t available = stream->available();
    if (available == 0) {
      if (!http.connected()) { mbedtls_sha256_free(&sha); Update.abort(); http.end(); fail("Connection dropped during download"); return; }
      if (millis() - lastData > 15000) { mbedtls_sha256_free(&sha); Update.abort(); http.end(); fail("Download timed out"); return; }
      delay(1); continue;
    }
    size_t toRead = available;
    if (toRead > sizeof(buffer)) toRead = sizeof(buffer);
    if (toRead > expectedSize_ - received) toRead = expectedSize_ - received;
    const size_t readBytes = stream->readBytes(buffer, toRead);
    if (readBytes == 0) continue;
    lastData = millis();
    mbedtls_sha256_update(&sha, buffer, readBytes);
    if (Update.write(buffer, readBytes) != readBytes) { mbedtls_sha256_free(&sha); Update.abort(); http.end(); fail("Flash write error"); return; }
    received += readBytes;
    progress_ = (int)((received * 100ULL) / expectedSize_);
    delay(1);
  }
  http.end();

  uint8_t hash[32]; mbedtls_sha256_finish(&sha, hash); mbedtls_sha256_free(&sha);
  const String actual = sha256Hex(hash);
  if (!actual.equalsIgnoreCase(expectedSha256_)) { Update.abort(); fail("SHA-256 mismatch, update discarded"); return; }

  if (!Update.end() || !Update.isFinished()) { fail(String("Update could not be finalized: ") + Update.errorString()); return; }

  progress_ = 100; state_ = "restarting"; message_ = "Update successful, restarting"; running_ = false;
  Serial.println("[RedButton] Firmware update successful, restarting");
  delay(1000);
  ESP.restart();
}
