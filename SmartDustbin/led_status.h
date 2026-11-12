#ifndef SMART_DUSTBIN_LED_STATUS_H
#define SMART_DUSTBIN_LED_STATUS_H

#include <Arduino.h>

enum class LedFillIndicatorState : uint8_t {
  Unknown = 0,
  Normal = 1,
  Warning = 2,
};

void ledStatusBegin();

// Uses WP-4 last valid fill %; safe to call every loop (outputs update on change).
void ledStatusUpdate();

LedFillIndicatorState ledStatusGetState();

bool ledStatusAreThresholdsValid();

bool ledStatusGetWhiteOn();

bool ledStatusGetYellowOn();

#endif
