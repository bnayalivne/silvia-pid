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
  int detected = 0;
  switch (machineState)
  {
  case STATE_INIT:
    if (currentTemp < (gTargetTemp - 10))
    {
      machineState = STATE_COLDSTART;
    }
    else if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = STATE_STEAM;
    }
    else
    {
      machineState = STATE_READY;
    }
    break;

  case STATE_COLDSTART:
    switch (machinestatecold)
    {
    case 0:
      if (currentTemp >= (gTargetTemp - 1) && currentTemp < 150)
      {
        machinestatecoldmillis = millis();
        machinestatecold = 10;
      }
      break;
    case 10:
      if (currentTemp < (gTargetTemp - 1))
      {
        machinestatecold = 0;
      }
      if (machinestatecoldmillis + 10 * 1000 < millis())
      {
        machineState = STATE_PREREADY;
      }
      break;
    }
    if (totalBrewTime > 0)
    {
      machineState = STATE_BREWING;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = STATE_STEAM;
    }
    break;

  case STATE_PREREADY:
    if (currentTemp >= gTargetTemp)
    {
      machineState = STATE_READY;
    }
    if (totalBrewTime > 0)
    {
      machineState = STATE_BREWING;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = STATE_STEAM;
    }
    // fall through to STATE_READY

  case STATE_READY:
    brewdetection(gTargetTemp);
    if (totalBrewTime > 0)
    {
      machineState = STATE_BREWING;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = STATE_STEAM;
    }
    break;

  case STATE_BREWING:
    detected = brewdetection(gTargetTemp);
    if (detected == 0)
    {
      machineState = STATE_READY;
    }
    else if (totalBrewTime > 35 * 1000)
    {
      machineState = STATE_POSTBREW;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = STATE_STEAM;
    }
    break;

  case STATE_POSTBREW:
    detected = brewdetection(gTargetTemp);
    if (detected == 0)
    {
      machineState = STATE_READY;
    }
    else
    {
      machineState = STATE_BREWING;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = STATE_STEAM;
    }
    // fall through to STATE_STEAM

  case STATE_STEAM:
    if (currentTemp < STEAM_TEMP_DETECTION)
    {
      machineState = STATE_CHILL;
    }
    break;

  case STATE_CHILL:
    if (heatrateaverage > 0 && currentTemp < (gTargetTemp + 2))
    {
      machineState = STATE_READY;
    }
    if (currentTemp >= STEAM_TEMP_DETECTION)
    {
      machineState = STATE_STEAM;
    }
    break;
  }
}
