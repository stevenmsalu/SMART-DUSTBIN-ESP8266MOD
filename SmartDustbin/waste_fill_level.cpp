#include <Arduino.h>

#include "config.h"
#include "waste_fill_level.h"

static bool s_calibrationValid = false;
static bool s_hasValidPercent = false;
static uint8_t s_lastValidFillPercent = 0;

static bool referencesAreValid() {
  return CONFIG_WASTE_EMPTY_REF_CM > CONFIG_WASTE_FULL_REF_CM;
}

static uint8_t computeFillPercentClamped(uint16_t measuredCm) {
  const uint16_t emptyCm = CONFIG_WASTE_EMPTY_REF_CM;
  const uint16_t fullCm = CONFIG_WASTE_FULL_REF_CM;
  const uint16_t spanCm = emptyCm - fullCm;

  int32_t numerator =
      static_cast<int32_t>(emptyCm - measuredCm) * 100;
  int32_t percent = numerator / static_cast<int32_t>(spanCm);

  if (percent < 0) {
    percent = 0;
  } else if (percent > 100) {
    percent = 100;
  }

  return static_cast<uint8_t>(percent);
}

void wasteFillLevelBegin() {
  s_calibrationValid = referencesAreValid();
  s_hasValidPercent = false;
  s_lastValidFillPercent = 0;
}

void wasteFillLevelApplyReading(const WasteSensorReading& reading) {
  if (!reading.valid) {
    return;
  }

  if (!s_calibrationValid) {
    return;
  }

  s_lastValidFillPercent = computeFillPercentClamped(reading.distanceCm);
  s_hasValidPercent = true;
}

bool wasteFillLevelIsCalibrationValid() {
  return s_calibrationValid;
}

bool wasteFillLevelHasValidPercent() {
  return s_hasValidPercent;
}

bool wasteFillLevelGetLastValidPercent(uint8_t& fillPercentOut) {
  if (!s_hasValidPercent) {
    return false;
  }
  fillPercentOut = s_lastValidFillPercent;
  return true;
}
