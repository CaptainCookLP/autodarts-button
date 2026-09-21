#pragma once
#include <Arduino.h>

struct FirmwareCheckResult {
  bool ok = false;
  bool available = false;
  String currentVersion;
  String latestVersion;
  size_t size = 0;
  String error;
};

// Checks GitHub Releases for a newer firmware.bin, verifies it via pinned TLS root + SHA-256, then flashes it.
class UpdateManager {
 public:
  explicit UpdateManager(const char* currentVersion) : currentVersion_(currentVersion) {}
  FirmwareCheckResult checkLatest();
  bool requestInstall();
  void loop();
  const String& state() const { return state_; }
  const String& message() const { return message_; }
  int progress() const { return progress_; }

 private:
  void performUpdate();
  void fail(const String& error);

  String currentVersion_;
  String downloadUrl_, expectedSha256_;
  size_t expectedSize_ = 0;
  bool available_ = false;
  bool pending_ = false, running_ = false;
  uint32_t pendingSince_ = 0;
  String state_ = "idle", message_;
  int progress_ = 0;
};
