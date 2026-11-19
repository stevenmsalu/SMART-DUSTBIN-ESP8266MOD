#include <Arduino.h>
#include <ESP8266WebServer.h>

#include "application_status.h"
#include "config.h"
#include "dashboard_page.h"
#include "http_api.h"
#include "wifi_manager.h"

static ESP8266WebServer s_server(CONFIG_HTTP_PORT);

static void appendQuoted(String& out, const char* value) {
  out += '\"';
  out += value;
  out += '\"';
}

static void appendLidState(String& out, LidPositionState state) {
  if (state == LidPositionState::Open) {
    appendQuoted(out, "OPEN");
  } else {
    appendQuoted(out, "CLOSED");
  }
}

static String buildStatusJson(const ApplicationStatusSnapshot& snap) {
  char ipBuf[16];
  const bool wifiUp = wifiManagerIsConnected();
  if (wifiUp) {
    wifiManagerGetIpAddress(ipBuf, sizeof(ipBuf));
  } else {
    ipBuf[0] = '\0';
  }

  String json;
  json.reserve(280);

  json += F("{\"device\":{\"uptime_ms\":");
  json += snap.uptimeMs;
  json += F("},\"approach\":{\"detected\":");
  json += snap.approachDetected ? F("true") : F("false");
  json += F("},\"lid\":{\"state\":");
  appendLidState(json, snap.lidState);
  json += F(",\"close_delay_pending\":");
  json += snap.lidCloseDelayPending ? F("true") : F("false");
  json += F(",\"servo_target_angle_deg\":");
  json += snap.servoTargetAngleDeg;
  json += F("},\"leds\":{\"white_on\":");
  json += snap.whiteLedOn ? F("true") : F("false");
  json += F("},\"network\":{\"wifi_connected\":");
  json += wifiUp ? F("true") : F("false");
  json += F(",\"ip\":");
  if (wifiUp && ipBuf[0] != '\0') {
    appendQuoted(json, ipBuf);
  } else {
    json += F("null");
  }
  json += F("}}");
  return json;
}

static String buildHealthJson(const ApplicationStatusSnapshot& snap) {
  char ipBuf[16];
  const bool wifiUp = wifiManagerIsConnected();
  if (wifiUp) {
    wifiManagerGetIpAddress(ipBuf, sizeof(ipBuf));
  } else {
    ipBuf[0] = '\0';
  }

  String json;
  json.reserve(160);

  json += F("{\"device\":{\"status\":");
  appendQuoted(json, "ok");
  json += F(",\"uptime_ms\":");
  json += snap.uptimeMs;
  json += F("},\"wifi\":{\"connected\":");
  json += wifiUp ? F("true") : F("false");
  json += F(",\"ip\":");
  if (wifiUp && ipBuf[0] != '\0') {
    appendQuoted(json, ipBuf);
  } else {
    json += F("null");
  }
  json += F("},\"features\":{\"monitoring_only\":true}}");
  return json;
}

static void handleDashboardRoot() {
  s_server.send_P(200, PSTR("text/html"), DASHBOARD_HTML);
  Serial.println(F("[http] GET /"));
}

static void handleApiStatus() {
  ApplicationStatusSnapshot snap;
  applicationStatusGetSnapshot(millis(), snap);

  const String json = buildStatusJson(snap);
  s_server.send(200, F("application/json"), json);
  Serial.println(F("[http] GET /api/status"));
}

static void handleApiHealth() {
  ApplicationStatusSnapshot snap;
  applicationStatusGetSnapshot(millis(), snap);

  const String json = buildHealthJson(snap);
  s_server.send(200, F("application/json"), json);
  Serial.println(F("[http] GET /api/health"));
}

void httpApiBegin() {
  s_server.on(F("/"), HTTP_GET, handleDashboardRoot);
  s_server.on(F("/api/status"), HTTP_GET, handleApiStatus);
  s_server.on(F("/api/health"), HTTP_GET, handleApiHealth);
  s_server.begin();
  Serial.print(F("[http] listening port "));
  Serial.println(CONFIG_HTTP_PORT);
}

void httpApiHandleClient() {
  s_server.handleClient();
}