#ifndef SIMULATION_MODE
#include <Adafruit_MAX31865.h>
#include <sensor.h>
#include <shared.h>

#define RREF      430.0
#define RNOMINAL  100.0

#define SIM_TIME 100
#define SIM_T_START 20
#define SIM_T_LOSS 2.9e-2
#define SIM_T_HEAT 0.7787

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(13, 14, 27, 26);

double currentTempRead = 0.0;

void setupSensor(){
    thermo.begin(MAX31865_3WIRE);
}
double getCurrentTemperature(){
    currentTempRead = thermo.temperature(RNOMINAL, RREF);
    uint8_t fault = thermo.readFault();
    if (fault) {
        Serial.print("Fault 0x"); Serial.println(fault, HEX);
        if (fault & MAX31865_FAULT_HIGHTHRESH) {
        Serial.println("RTD High Threshold"); 
        }
        if (fault & MAX31865_FAULT_LOWTHRESH) {
        Serial.println("RTD Low Threshold"); 
        }
        if (fault & MAX31865_FAULT_REFINLOW) {
        Serial.println("REFIN- > 0.85 x Bias"); 
        }
        if (fault & MAX31865_FAULT_REFINHIGH) {
        Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
        }
        if (fault & MAX31865_FAULT_RTDINLOW) {
        Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
        }
        if (fault & MAX31865_FAULT_OVUV) {
        Serial.println("Under/Over voltage"); 
        }
        thermo.clearFault();
    }
    return currentTempRead;
}
#endif