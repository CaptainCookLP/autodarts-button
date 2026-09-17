#pragma once
#include <Arduino.h>

enum class ActionType : uint8_t { Disabled = 0, Hid = 1, SerialEvent = 2 };
enum Modifier : uint8_t { ModCtrl = 1, ModShift = 2, ModAlt = 4, ModGui = 8 };

struct AppConfig {
  static constexpr uint16_t CURRENT_VERSION = 1;
  uint16_t version = CURRENT_VERSION;
  String deviceName = "redbutton";
  String wifiSsid;
  String wifiPassword;
  ActionType actionType = ActionType::Hid;
  String hidKey = "K";
  uint8_t modifiers = ModCtrl | ModShift;
  String serialAction = "custom_1";
};

