#include <Arduino.h>
#include <shared.h>

#define HEATER_INTERVAL 1000
#define HEAT_RELAY_PIN  19

float heatcycles;
bool heaterState = 0;
unsigned long heatCurrentTime = 0, heatLastTime = 0; 

void setupHeater() {
    pinMode(HEAT_RELAY_PIN, OUTPUT);
}

void _turnHeatElementOnOff(bool on) {
    digitalWrite(HEAT_RELAY_PIN, on);
    heaterState = on;
}

void updateHeater() {
  heatCurrentTime = time_now;
  bool newStatus = false;
  bool shouldUpdate = false;
  if(heatCurrentTime - heatLastTime >= HEATER_INTERVAL or heatLastTime > heatCurrentTime) { //second statement prevents overflow errors
    // begin cycle
    newStatus = true;
    shouldUpdate = true;
    heatLastTime = heatCurrentTime;
  }
  if (heatCurrentTime - heatLastTime >= heatcycles) {
    shouldUpdate = true;
    newStatus = false;
  }
  if(shouldUpdate){
    _turnHeatElementOnOff(newStatus);
  }
}

void setHeatPowerPercentage(float power) {
  if (power < 0.0) {
    power = 0.0;
  }  
  if (power > 1000.0) {
    power = 1000.0;
  }
  heatcycles = power;
}

float getHeatCycles() {
  return heatcycles;
}