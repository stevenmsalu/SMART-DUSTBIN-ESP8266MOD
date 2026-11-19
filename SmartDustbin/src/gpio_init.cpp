#include <Arduino.h>

#include "gpio_init.h"
#include "../pins.h"

void gpioInitSafeOutputsAndInputs() {
  // HC-SR04: TRIG idle LOW; no measurement pulses in WP-1
  pinMode(PIN_APPROACH_TRIG, OUTPUT);
  digitalWrite(PIN_APPROACH_TRIG, LOW);

  pinMode(PIN_APPROACH_ECHO, INPUT);

  // Servo D7: initialised by lid_control (WP-5); leave pin untouched here.

  // D0 white LED: configured by white_indicator.

  // D4 / GPIO2 buzzer: not configured (physically disconnected; boot strap)
}
