#include <Arduino.h>
#include <state_machine.h>
#include <globals.h>
#include <brewdetection.h>

// Internal state for cold start tracking
static int machinestatecold = 0;
static unsigned long machinestatecoldmillis = 0;

extern double currentTemp;

void checkMachineState()
{
  /*
  00 = init
  10 = cold start
  19 = Setpoint -1 Celsius
  20 = Setpoint
  30 = Brewing
  35 = After brewing
  40 = Steam
  45 = Chill after steam
  */
  int detected = 0;
  switch (machineState)
  {
  // init
  case 0:
    if (currentTemp < (gTargetTemp - 10))
    {
      machineState = 10;
    }
    else if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = 40;
    }
    else
    {
      machineState = 20;
    }
    break;
  // cold start
  case 10:
    switch (machinestatecold)
    {
    case 0:
      if (currentTemp >= (gTargetTemp - 1) && currentTemp < 150)
      {
        machinestatecoldmillis = millis(); // get millis for interval calc
        machinestatecold = 10;             // new state
      }
      break;
    case 10:
      if (currentTemp < (gTargetTemp - 1))
      {
        machinestatecold = 0; //  Input was only one time above BrewSetPoint, reset machinestatecold
      }
      if (machinestatecoldmillis + 10 * 1000 < millis()) // 10 sec Input above BrewSetPoint, no set new state
      {
        machineState = 19;
      }
      break;
    }
    if (totalBrewTime > 0)
    {
      machineState = 30;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = 40;
    }
    break;
  // Setpoint -1 Celsius
  case 19:
    if (currentTemp >= gTargetTemp)
    {
      machineState = 20;
    }
    if (totalBrewTime > 0)
    {
      machineState = 30;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = 40;
    }
  case 20:
    brewdetection(gTargetTemp);
    if (totalBrewTime > 0)
    {
      machineState = 30;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = 40;
    }
    break;
  case 30:
    detected = brewdetection(gTargetTemp);
    if (detected == 0)
    {
      machineState = 20;
    }
    else if (totalBrewTime > 35 * 1000)
    {
      // after 35 seconds
      machineState = 35;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = 40;
    }
    break;
  case 35:
    detected = brewdetection(gTargetTemp);
    if (detected == 0)
    {
      machineState = 20;
    }
    else
    {
      machineState = 30;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = 40;
    }
  case 40:
    if (currentTemp < STEAM_TEMP_DETECTION)
    {
      machineState = 45;
    }
    break;
  case 45:
    if (heatrateaverage > 0 && currentTemp < (gTargetTemp + 2))
    {
      machineState = 20;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = 40;
    }
    break;
  }
}
