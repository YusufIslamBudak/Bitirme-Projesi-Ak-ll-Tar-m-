#ifndef GREENHOUSE_CONTROL_H
#define GREENHOUSE_CONTROL_H

#include <Arduino.h>

// Sera kontrol fonksiyonlari
void controlGreenhouse();
int checkGreenhouseConditions();
void setRoofPosition(int position, String reason);

#endif
