#ifndef SMART_DUSTBIN_HCSR04_COMMON_H
#define SMART_DUSTBIN_HCSR04_COMMON_H

#include <Arduino.h>

struct Hcsr04Reading {
  bool valid;
  bool timedOut;
  bool outOfRange;
  uint16_t distanceCm;
};

Hcsr04Reading hcsr04Measure(uint8_t trigPin, uint8_t echoPin,
                            unsigned long pulseTimeoutUs,
                            uint16_t minValidCm, uint16_t maxValidCm,
                            unsigned long usPerCm);

#endif
