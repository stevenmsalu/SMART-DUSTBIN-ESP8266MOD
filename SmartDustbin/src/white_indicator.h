#ifndef SMART_DUSTBIN_WHITE_INDICATOR_H
#define SMART_DUSTBIN_WHITE_INDICATOR_H

#include <Arduino.h>

void whiteIndicatorBegin();

// Call after lidControlUpdate() each loop; GPIO changes only on transition.
void whiteIndicatorUpdate();

bool whiteIndicatorIsOn();

#endif