#ifndef SMART_DUSTBIN_WASTE_FILL_LEVEL_H
#define SMART_DUSTBIN_WASTE_FILL_LEVEL_H

#include <Arduino.h>

#include "waste_sensor.h"

void wasteFillLevelBegin();

// Call after each waste measurement; invalid readings do not update fill %.
void wasteFillLevelApplyReading(const WasteSensorReading& reading);

bool wasteFillLevelIsCalibrationValid();

bool wasteFillLevelHasValidPercent();

bool wasteFillLevelGetLastValidPercent(uint8_t& fillPercentOut);

#endif
