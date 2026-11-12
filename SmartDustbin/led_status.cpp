#include <Arduino.h>

#include "config.h"
#include "led_status.h"
#include "pins.h"
#include "waste_fill_level.h"

static bool s_thresholdsValid = false;
static bool s_warningLatched = false;
static LedFillIndicatorState s_state = LedFillIndicatorState::Unknown;
static bool s_whiteOn = false;
static bool s_yellowOn = false;

static bool validateThresholdConfig() {
  if (CONFIG_WARN_FILL_PERCENT > 100 || CONFIG_WARN_CLEAR_PERCENT > 100) {
    return false;
  }
  return CONFIG_WARN_CLEAR_PERCENT < CONFIG_WARN_FILL_PERCENT;
}

static void setOutputs(bool whiteOn, bool yellowOn) {
  digitalWrite(PIN_LED_WHITE, whiteOn ? HIGH : LOW);
  digitalWrite(PIN_LED_YELLOW, yellowOn ? HIGH : LOW);
  s_whiteOn = whiteOn;
  s_yellowOn = yellowOn;
}

static const __FlashStringHelper* stateLabel(LedFillIndicatorState st) {
  switch (st) {
    case LedFillIndicatorState::Normal:
      return F("NORMAL");
    case LedFillIndicatorState::Warning:
      return F("WARNING");
    default:
      return F("UNKNOWN");
  }
}

static void logLedStateChange(LedFillIndicatorState st, uint8_t fillPct,
                              bool fillKnown) {
  Serial.print(F("[led] state="));
  Serial.print(stateLabel(st));
  Serial.print(F(" white="));
  Serial.print(s_whiteOn ? 1 : 0);
  Serial.print(F(" yellow="));
  Serial.print(s_yellowOn ? 1 : 0);
  Serial.print(F(" warn_pct="));
  Serial.print(CONFIG_WARN_FILL_PERCENT);
  Serial.print(F(" clear_pct="));
  Serial.print(CONFIG_WARN_CLEAR_PERCENT);
  if (fillKnown) {
    Serial.print(F(" fill_pct="));
    Serial.print(fillPct);
  } else {
    Serial.print(F(" fill_pct=none"));
  }
  Serial.println();
}

static void applyState(LedFillIndicatorState st, uint8_t fillPct,
                       bool fillKnown) {
  if (st == s_state &&
      ((st == LedFillIndicatorState::Unknown &&
        !s_whiteOn && !s_yellowOn) ||
       (st == LedFillIndicatorState::Normal && s_whiteOn && !s_yellowOn) ||
       (st == LedFillIndicatorState::Warning && !s_whiteOn && s_yellowOn))) {
    return;
  }

  s_state = st;

  switch (st) {
    case LedFillIndicatorState::Unknown:
      setOutputs(false, false);
      break;
    case LedFillIndicatorState::Normal:
      setOutputs(true, false);
      break;
    case LedFillIndicatorState::Warning:
      setOutputs(false, true);
      break;
  }

  logLedStateChange(st, fillPct, fillKnown);
}

void ledStatusBegin() {
  pinMode(PIN_LED_WHITE, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);

  // After flash boot: indicators OFF (active HIGH wiring).
  setOutputs(false, false);

  s_thresholdsValid = validateThresholdConfig();
  s_warningLatched = false;
  s_state = LedFillIndicatorState::Unknown;

  if (!s_thresholdsValid) {
    Serial.println(F("[led] ERROR: thresholds must be 0-100 and clear < warn"));
  } else {
    Serial.print(F("[led] thresholds warn="));
    Serial.print(CONFIG_WARN_FILL_PERCENT);
    Serial.print(F("% clear="));
    Serial.print(CONFIG_WARN_CLEAR_PERCENT);
    Serial.println(F("% (provisional)"));
    Serial.println(F("[led] GPIO0/D3: boot HIGH required — validate cold boot on bench."));
  }
}

void ledStatusUpdate() {
  if (!s_thresholdsValid) {
    applyState(LedFillIndicatorState::Unknown, 0, false);
    return;
  }

  if (!wasteFillLevelHasValidPercent()) {
    applyState(LedFillIndicatorState::Unknown, 0, false);
    return;
  }

  uint8_t fillPct = 0;
  if (!wasteFillLevelGetLastValidPercent(fillPct)) {
    applyState(LedFillIndicatorState::Unknown, 0, false);
    return;
  }

  if (!s_warningLatched) {
    if (fillPct >= CONFIG_WARN_FILL_PERCENT) {
      s_warningLatched = true;
    }
  } else if (fillPct <= CONFIG_WARN_CLEAR_PERCENT) {
    s_warningLatched = false;
  }

  const LedFillIndicatorState next =
      s_warningLatched ? LedFillIndicatorState::Warning
                       : LedFillIndicatorState::Normal;

  applyState(next, fillPct, true);
}

LedFillIndicatorState ledStatusGetState() {
  return s_state;
}

bool ledStatusAreThresholdsValid() {
  return s_thresholdsValid;
}

bool ledStatusGetWhiteOn() {
  return s_whiteOn;
}

bool ledStatusGetYellowOn() {
  return s_yellowOn;
}
