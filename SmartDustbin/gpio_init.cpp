#include <Arduino.h>

#include "gpio_init.h"
#include "pins.h"

void gpioInitSafeOutputsAndInputs() {
  // HC-SR04: TRIG idle LOW; no measurement pulses in WP-1
  pinMode(PIN_APPROACH_TRIG, OUTPUT);
  digitalWrite(PIN_APPROACH_TRIG, LOW);
  pinMode(PIN_WASTE_TRIG, OUTPUT);
  digitalWrite(PIN_WASTE_TRIG, LOW);

  pinMode(PIN_APPROACH_ECHO, INPUT);
  pinMode(PIN_WASTE_ECHO, INPUT);

  // Servo D7: initialised by lid_control (WP-5); leave pin untouched here.

  // Indicators off (active HIGH wiring: LOW = off)
  pinMode(PIN_LED_WHITE, OUTPUT);
  digitalWrite(PIN_LED_WHITE, LOW);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  digitalWrite(PIN_LED_YELLOW, LOW);

  // D4 / GPIO2 buzzer: not configured in WP-2 (physically disconnected; boot strap)
}
