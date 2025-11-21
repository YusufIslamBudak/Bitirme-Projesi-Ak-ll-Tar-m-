#ifndef IRRIGATION_CONTROL_H
#define IRRIGATION_CONTROL_H

#include <Arduino.h>

// Sulama kontrol fonksiyonlari
void controlIrrigation();
bool checkIrrigationNeeded();
void setPumpState(bool state, int duration, String reason);

#endif
