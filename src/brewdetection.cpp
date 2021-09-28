#include <Arduino.h>
#include <shared.h>
#include <sensor.h>

double totalBrewTime = 0;        //total brewed time
double lastbezugszeitMillis = 0; // for shottimer delay after disarmed button
double lastbezugszeit = 0;
int timerBrewdetection = 0; // flag is set if brew was detected
unsigned long timeBrewdetection = 0;

double brewtimersoftware = 45; // 20-5 for detection
double brewboarder = 150;      // border for the detection, be carefull: to low: risk of wrong brew detection and rising temperature

/********************************************************
   moving average - brewdetection
*****************************************************/
const int numReadings = 15;              // number of values per Array
double readingstemp[numReadings];        // the readings from Temp
unsigned long readingstime[numReadings]; // the readings from time
double readingchangerate[numReadings];

int readIndex = 1;          // the index of the current reading
double total = 0;           // total sum of readingchangerate[]
double heatrateaverage = 0; // the average over the numReadings
double changerate = 0;      // local change rate of temprature
double heatrateaveragemin = 0;
int firstreading = 1;       // Ini of the field, also used for sensor check

int brewdetection(double BrewSetPoint)
{

    // Bezugstimmer für SW aktivieren
    if (timerBrewdetection == 1)
    {
        totalBrewTime = millis() - timeBrewdetection;
    }
    // Bezugstimmer für SW deaktivieren nach ende BD PID
    if (millis() - timeBrewdetection > brewtimersoftware * 1000 && timerBrewdetection == 1)
    {
        Serial.println("SW Brew stopped");
        timerBrewdetection = 0; //rearm brewdetection
        if (machineState != 30) // Bei Onlypid = 1, totalBrewTime > 0, no reset of bezugsZeit in case of brewing.
        {
            totalBrewTime = 0;
        }
    }

    // Activate the BD
    if (heatrateaverage <= -brewboarder && timerBrewdetection == 0 && (fabs(currentTemp - BrewSetPoint) < 5)) // BD PID only +/- 4 Grad Celsius, no detection if HW was active
    {
        Serial.println("SW Brew detected");
        timeBrewdetection = millis();
        timerBrewdetection = 1;
    }

    return timerBrewdetection;
}

/********************************************************
  Moving average - brewdetection (SW)
*****************************************************/
void movAvg()
{
    if (firstreading == 1)
    {
        for (int thisReading = 0; thisReading < numReadings; thisReading++)
        {
            readingstemp[thisReading] = currentTemp;
            readingstime[thisReading] = 0;
            readingchangerate[thisReading] = 0;
        }
        firstreading = 0;
    }

    readingstime[readIndex] = millis();
    readingstemp[readIndex] = currentTemp;

    if (readIndex == numReadings - 1)
    {
        changerate = (readingstemp[numReadings - 1] - readingstemp[0]) / (readingstime[numReadings - 1] - readingstime[0]) * 10000;
    }
    else
    {
        changerate = (readingstemp[readIndex] - readingstemp[readIndex + 1]) / (readingstime[readIndex] - readingstime[readIndex + 1]) * 10000;
    }

    readingchangerate[readIndex] = changerate;
    total = 0;
    for (int i = 0; i < numReadings; i++)
    {
        total += readingchangerate[i];
    }

    heatrateaverage = total / numReadings * 100;
    if (heatrateaveragemin > heatrateaverage)
    {
        heatrateaveragemin = heatrateaverage;
    }

    if (readIndex >= numReadings - 1)
    {
        // ...wrap around to the beginning:
        readIndex = 0;
    }
    else
    {
        readIndex++;
    }
}
