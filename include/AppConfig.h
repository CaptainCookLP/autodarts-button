#pragma once
#include <Arduino.h>

enum class ActionType : uint8_t { Disabled = 0, Hid = 1, SerialEvent = 2 };
enum Modifier : uint8_t { ModCtrl = 1, ModShift = 2, ModAlt = 4, ModGui = 8 };

struct ButtonConfig {
  int8_t gpio = -1;
  ActionType actionType = ActionType::Disabled;
  String hidKey = "K";
  uint8_t modifiers = ModCtrl | ModShift;
  String serialAction = "custom_1";
};

struct AppConfig {
  static constexpr uint16_t CURRENT_VERSION = 2;
  static constexpr uint8_t MAX_BUTTONS = 8;
  uint16_t version = CURRENT_VERSION;
  String deviceName = "redbutton";
  String wifiSsid;
  String wifiPassword;
  uint8_t buttonCount = 1;
  ButtonConfig buttons[MAX_BUTTONS];
  AppConfig() { buttons[0].gpio = 2; buttons[0].actionType = ActionType::Hid; } // GPIO2 = XIAO pin D1
};

