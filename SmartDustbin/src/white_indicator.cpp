#include <Arduino.h>

#include "lid_control.h"
#include "../pins.h"
#include "white_indicator.h"

static bool s_on = false;

void whiteIndicatorBegin() {
  pinMode(PIN_LED_WHITE, OUTPUT);
  digitalWrite(PIN_LED_WHITE, LOW);
  s_on = false;
  Serial.println(F("[white] init OFF (D0/GPIO16)"));
}

void whiteIndicatorUpdate() {
  const bool shouldOn = (lidControlGetLidState() == LidPositionState::Open);

  if (shouldOn == s_on) {
    return;
  }

  s_on = shouldOn;
  digitalWrite(PIN_LED_WHITE, shouldOn ? HIGH : LOW);

  Serial.print(F("[white] "));
  Serial.println(shouldOn ? F("ON (lid open)") : F("OFF (lid closed)"));
}

bool whiteIndicatorIsOn() {
  return s_on;
}