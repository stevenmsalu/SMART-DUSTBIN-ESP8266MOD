#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "../config.h"
#include "../wifi_config.h"
#include "wifi_manager.h"

static WifiManagerState s_state = WifiManagerState::Disconnected;
static bool s_loggedConnected = false;
static unsigned long s_nextReconnectAttemptMs = 0;

void wifiManagerBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);

  s_state = WifiManagerState::Connecting;
  s_loggedConnected = false;
  s_nextReconnectAttemptMs = 0;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println(F("[wifi] connect initiated (non-blocking)"));
}

void wifiManagerUpdate(unsigned long nowMs) {
  if (WiFi.status() == WL_CONNECTED) {
    s_state = WifiManagerState::Connected;
    if (!s_loggedConnected) {
      s_loggedConnected = true;
      Serial.print(F("[wifi] connected ip="));
      Serial.println(WiFi.localIP());
    }
    return;
  }

  if (s_loggedConnected) {
    s_loggedConnected = false;
    Serial.println(F("[wifi] disconnected"));
  }

  s_state = WifiManagerState::Connecting;

  if (s_nextReconnectAttemptMs != 0 && nowMs < s_nextReconnectAttemptMs) {
    return;
  }

  if (s_nextReconnectAttemptMs != 0) {
    Serial.println(F("[wifi] reconnect attempt"));
    WiFi.disconnect();
    yield();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  s_nextReconnectAttemptMs = nowMs + CONFIG_WIFI_RECONNECT_INTERVAL_MS;
}

bool wifiManagerIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

WifiManagerState wifiManagerGetState() {
  return s_state;
}

bool wifiManagerGetIpAddress(char* buffer, size_t bufferLen) {
  if (buffer == nullptr || bufferLen == 0) {
    return false;
  }
  buffer[0] = '\0';
  if (!wifiManagerIsConnected()) {
    return false;
  }
  const IPAddress ip = WiFi.localIP();
  snprintf(buffer, bufferLen, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return true;
}

static const __FlashStringHelper* wifiStatusLabel(int status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return F("WL_IDLE_STATUS");
    case WL_NO_SSID_AVAIL:
      return F("WL_NO_SSID_AVAIL");
    case WL_SCAN_COMPLETED:
      return F("WL_SCAN_COMPLETED");
    case WL_CONNECTED:
      return F("WL_CONNECTED");
    case WL_CONNECT_FAILED:
      return F("WL_CONNECT_FAILED");
    case WL_CONNECTION_LOST:
      return F("WL_CONNECTION_LOST");
    case WL_DISCONNECTED:
      return F("WL_DISCONNECTED");
    case WL_NO_SHIELD:
      return F("WL_NO_SHIELD");
    default:
      return F("WL_UNKNOWN");
  }
}

void wifiManagerPrintPeriodicDiagnostic() {
  const int status = WiFi.status();
  const bool connected = (status == WL_CONNECTED);

  Serial.print(F("[wifi] status="));
  Serial.print(status);
  Serial.print(F(" ("));
  Serial.print(wifiStatusLabel(status));
  Serial.print(F(") connected="));
  Serial.println(connected ? F("true") : F("false"));

  if (!connected) {
    return;
  }

  Serial.print(F("[wifi] ip="));
  Serial.println(WiFi.localIP());
  Serial.print(F("[wifi] gateway="));
  Serial.println(WiFi.gatewayIP());
  Serial.print(F("[wifi] subnet="));
  Serial.println(WiFi.subnetMask());
  Serial.print(F("[wifi] rssi="));
  Serial.println(WiFi.RSSI());
}
