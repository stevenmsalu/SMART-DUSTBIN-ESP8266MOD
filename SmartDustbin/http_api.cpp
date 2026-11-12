#include <Arduino.h>
#include <ESP8266WebServer.h>

#include "application_status.h"
#include "config.h"
#include "dashboard_page.h"
#include "http_api.h"
#include "wifi_manager.h"

static ESP8266WebServer s_server(CONFIG_HTTP_PORT);

static void writeQuoted(Print& out, const char* value) {
  out.print('\"');
  out.print(value);
  out.print('\"');
}

static void writeLidState(Print& out, LidPositionState state) {
  if (state == LidPositionState::Open) {
    writeQuoted(out, "OPEN");
  } else {
    writeQuoted(out, "CLOSED");
  }
}

static void writeLedIndicator(Print& out, LedFillIndicatorState state) {
  switch (state) {
    case LedFillIndicatorState::Normal:
      writeQuoted(out, "NORMAL");
      break;
    case LedFillIndicatorState::Warning:
      writeQuoted(out, "WARNING");
      break;
    default:
      writeQuoted(out, "UNKNOWN");
      break;
  }
}

static void writeLatestInvalidReason(Print& out,
                                     const ApplicationStatusSnapshot& snap) {
  if (snap.wasteLatestTimedOut) {
    writeQuoted(out, "timeout");
  } else if (snap.wasteLatestOutOfRange) {
    writeQuoted(out, "out_of_range");
  } else {
    writeQuoted(out, "none");
  }
}

static void writeStatusJson(Print& out, const ApplicationStatusSnapshot& snap) {
  char ipBuf[16];
  const bool wifiUp = wifiManagerIsConnected();
  if (wifiUp) {
    wifiManagerGetIpAddress(ipBuf, sizeof(ipBuf));
  } else {
    ipBuf[0] = '\0';
  }

  out.print(F("{\"device\":{\"uptime_ms\":"));
  out.print(snap.uptimeMs);
  out.print(F("},\"approach\":{\"detected\":"));
  out.print(snap.approachDetected ? F("true") : F("false"));
  out.print(F("},\"waste\":{\"last_valid_distance_cm\":{\"available\":"));
  out.print(snap.hasWasteDistanceLastValid ? F("true") : F("false"));
  if (snap.hasWasteDistanceLastValid) {
    out.print(F(",\"value\":"));
    out.print(snap.wasteDistanceLastValidCm);
  }
  out.print(F("},\"latest_attempt\":{\"valid\":"));
  out.print(snap.wasteLatestAttemptValid ? F("true") : F("false"));
  if (!snap.wasteLatestAttemptValid) {
    out.print(F(",\"reason\":"));
    writeLatestInvalidReason(out, snap);
  }
  out.print(F("}},\"fill\":{\"last_valid_percent\":{\"available\":"));
  out.print(snap.hasFillPercentLastValid ? F("true") : F("false"));
  if (snap.hasFillPercentLastValid) {
    out.print(F(",\"value\":"));
    out.print(snap.fillPercentLastValid);
  }
  out.print(F("}},\"lid\":{\"state\":"));
  writeLidState(out, snap.lidState);
  out.print(F(",\"close_delay_pending\":"));
  out.print(snap.lidCloseDelayPending ? F("true") : F("false"));
  out.print(F(",\"servo_target_angle_deg\":"));
  out.print(snap.servoTargetAngleDeg);
  out.print(F("},\"leds\":{\"indicator\":"));
  writeLedIndicator(out, snap.ledIndicatorState);
  out.print(F(",\"white_on\":"));
  out.print(snap.whiteLedOn ? F("true") : F("false"));
  out.print(F(",\"yellow_on\":"));
  out.print(snap.yellowLedOn ? F("true") : F("false"));
  out.print(F("},\"network\":{\"wifi_connected\":"));
  out.print(wifiUp ? F("true") : F("false"));
  out.print(F(",\"ip\":"));
  if (wifiUp && ipBuf[0] != '\0') {
    writeQuoted(out, ipBuf);
  } else {
    out.print(F("null"));
  }
  out.print(F("}}"));
}

static void handleDashboardRoot() {
  s_server.send_P(200, PSTR("text/html"), DASHBOARD_HTML);
  Serial.println(F("[http] GET /"));
}

static void handleApiStatus() {
  ApplicationStatusSnapshot snap;
  applicationStatusGetSnapshot(millis(), snap);

  s_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  s_server.send(200, F("application/json"), F(""));
  writeStatusJson(s_server.client(), snap);
  Serial.println(F("[http] GET /api/status"));
}

static void handleApiHealth() {
  ApplicationStatusSnapshot snap;
  applicationStatusGetSnapshot(millis(), snap);

  char ipBuf[16];
  const bool wifiUp = wifiManagerIsConnected();
  if (wifiUp) {
    wifiManagerGetIpAddress(ipBuf, sizeof(ipBuf));
  } else {
    ipBuf[0] = '\0';
  }

  s_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  s_server.send(200, F("application/json"), F(""));
  Print& out = s_server.client();
  out.print(F("{\"device\":{\"status\":"));
  writeQuoted(out, "ok");
  out.print(F(",\"uptime_ms\":"));
  out.print(snap.uptimeMs);
  out.print(F("},\"wifi\":{\"connected\":"));
  out.print(wifiUp ? F("true") : F("false"));
  out.print(F(",\"ip\":"));
  if (wifiUp && ipBuf[0] != '\0') {
    writeQuoted(out, ipBuf);
  } else {
    out.print(F("null"));
  }
  out.print(F("},\"features\":{\"monitoring_only\":true}}"));
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
