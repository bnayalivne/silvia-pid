#include <Arduino.h>
#include <web.h>
#include <shared.h>
#include <sensor.h>
#include <heater.h>
#include <PID_v1.h>
#include <config.h>
#include <brewdetection.h>
#include <state_machine.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <Wire.h>

#include "screen.h"

#define PID_INTERVAL 1000
//
// STANDARD reset values based on Silvia PID
//

double currentTemp = 0;
int machineState = STATE_INIT;
int previousMachineState = STATE_INIT;

double gTargetTemp = S_TSET;
double gOvershoot = S_TBAND;
double gOutputPwr = 0.0;
double gP = S_P, gI = S_I, gD = S_D;
double gaP = S_aP, gaI = S_aI, gaD = S_aD;

boolean overShootMode = false;
unsigned long time_now = 0;
unsigned long time_last = 0;

double aggKp = AGGKP;
double aggTn = AGGTN;
double aggTv = AGGTV;
double startKp = STARTKP;
double startTn = STARTTN;

double aggbKp = AGGBKP;
double aggbTn = AGGBTN;
double aggbTv = AGGBTV;
double aggbKi = 0;
double aggbKd = aggbTv * aggbKp;

double startKi = startKp / startTn;
double aggKi = aggKp / aggTn;
double aggKd = aggTv * aggKp;

PID ESPPID(&currentTemp, &gOutputPwr, &gTargetTemp, aggKp, aggKi, aggKd, 1, DIRECT);

void loopPID()
{
  // Cold start states - use aggressive tuning
  if (machineState == STATE_INIT || machineState == STATE_COLDSTART || machineState == STATE_PREREADY)
  {
    if (startTn != 0)
    {
      startKi = startKp / startTn;
    }
    else
    {
      startKi = 0;
    }
    ESPPID.SetTunings(startKp, startKi, 0, P_ON_M);
  }
  // Ready state - normal PID
  if (machineState == STATE_READY)
  {
    if (aggTn != 0)
    {
      aggKi = aggKp / aggTn;
    }
    else
    {
      aggKi = 0;
    }
    aggKd = aggTv * aggKp;
    ESPPID.SetTunings(aggKp, aggKi, aggKd, 1);
  }
  // Brewing states - brew detection PID
  if (machineState == STATE_BREWING || machineState == STATE_POSTBREW)
  {
    if (aggbTn != 0)
    {
      aggbKi = aggbKp / aggbTn;
    }
    else
    {
      aggbKi = 0;
    }
    aggbKd = aggbTv * aggbKp;
    ESPPID.SetTunings(aggbKp, aggbKi, aggbKd, 1);
  }
  // Steam mode
  if (machineState == STATE_STEAM)
  {
    ESPPID.SetTunings(150, 0, 0, 1);
  }
  // Chill mode after steam
  if (machineState == STATE_CHILL)
  {
    if (aggbTn != 0)
    {
      aggbKi = aggbKp / aggbTn;
    }
    else
    {
      aggbKi = 0;
    }
    aggbKd = aggbTv * aggbKp;
    ESPPID.SetTunings(aggbKp, aggbKi, aggbKd, 1);
  }
}

void setupOTA()
{
  ArduinoOTA
      .onStart([]()
               {
                 String type;
                 if (ArduinoOTA.getCommand() == U_FLASH)
                   type = "sketch";
                 else // U_SPIFFS
                   type = "filesystem";

                 // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
                 Serial.printf("Error[%u]: ", error);
                 if (error == OTA_AUTH_ERROR)
                   Serial.println("Auth Failed");
                 else if (error == OTA_BEGIN_ERROR)
                   Serial.println("Begin Failed");
                 else if (error == OTA_CONNECT_ERROR)
                   Serial.println("Connect Failed");
                 else if (error == OTA_RECEIVE_ERROR)
                   Serial.println("Receive Failed");
                 else if (error == OTA_END_ERROR)
                   Serial.println("End Failed"); });

  ArduinoOTA.begin();
}

void setup()
{
  Serial.begin(115200);

  Serial.println("Mounting SPIFFS...");
  if (!prepareFS())
  {
    Serial.println("Failed to mount SPIFFS !");
  }
  else
  {
    Serial.println("Mounted.");
  }
  Serial.println("Loading config...");
  if (!loadConfig())
  {
    Serial.println("Failed to load config. Using default values and creating config...");
    if (!saveConfig())
    {
      Serial.println("Failed to save config");
    }
    else
    {
      Serial.println("Config saved");
    }
  }
  else
  {
    Serial.println("Config loaded");
  }

  setupWeb();
  setupSensor();
  setupHeater();
  setupScreen();
  setupOTA();

  // start PID
  ESPPID.SetTunings(gP, gI, gD);
  ESPPID.SetSampleTime(PID_INTERVAL);
  ESPPID.SetOutputLimits(0, PID_INTERVAL);
  ESPPID.SetMode(AUTOMATIC);

  time_now = millis();
  time_last = time_now;
}

void loop()
{
  time_now = millis();
  ArduinoOTA.handle();

  currentTemp = getCurrentTemperature();

  movAvg();
  checkMachineState();

  loopPID();
  if (ESPPID.Compute() == true)
  {
    setHeatPowerPercentage(gOutputPwr);
  }
  updateHeater();

  updateScreen();
}