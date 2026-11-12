#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "config.h"
#include "wifi_config.h"
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
