#ifndef SMART_DUSTBIN_WIFI_MANAGER_H
#define SMART_DUSTBIN_WIFI_MANAGER_H

#include <Arduino.h>

enum class WifiManagerState : uint8_t {
  Disconnected = 0,
  Connecting = 1,
  Connected = 2,
};

void wifiManagerBegin();

void wifiManagerUpdate(unsigned long nowMs);

bool wifiManagerIsConnected();

WifiManagerState wifiManagerGetState();

// Writes dotted-quad or empty string; returns false if not connected.
bool wifiManagerGetIpAddress(char* buffer, size_t bufferLen);

#endif
