#include <Arduino.h>
#include <web.h>
#include <shared.h>
#include <sensor.h>
#include <heater.h>
#include <PID_v1.h>
#include <config.h>
#include <brewdetection.h>
#include <ArduinoOTA.h>

#define PID_INTERVAL 1000
//
// STANDARD reset values based on Silvia PID
//

double currentTemp = 0;
int machineState = 0;
int previousMachineState = 0;

double gTargetTemp = S_TSET;
double gOvershoot = S_TBAND;
double gOutputPwr = 0.0;
double gP = S_P, gI = S_I, gD = S_D;
double gaP = S_aP, gaI = S_aI, gaD = S_aD;
const int steamTempDetection = 120;

boolean overShootMode = false;
unsigned long time_now = 0;
unsigned long time_last = 0;

int machinestatecold = 0;
unsigned long machinestatecoldmillis = 0;

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
  if (machineState == 0 || machineState == 10 || machineState == 19) // Cold Start states
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
    // normal PID
  }
  if (machineState == 20)
  { //Prevent overwriting of brewdetection values
    // calc ki, kd
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
  // BD PID
  if (machineState >= 30 && machineState <= 35)
  {
    // calc ki, kd
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
  if (machineState == 40) // STEAM
  {
    ESPPID.SetTunings(150, 0, 0, 1);
  }
  if (machineState == 45) // chill-mode after steam
  {
    // calc ki, kd
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
  */
  int detected = 0;
  switch (machineState)
  {
  //init
  case 0:
    if (currentTemp < (gTargetTemp - 10))
    {
      machineState = 10;
    }
    else if (currentTemp >= steamTempDetection)
    {
      machineState = 40;
    }
    else
    {
      machineState = 20;
    }
    break;
  //cold start
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
    if (currentTemp >= steamTempDetection)
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
    if (currentTemp >= steamTempDetection)
    {
      machineState = 40;
    }
  case 20:
    brewdetection(gTargetTemp);
    if (totalBrewTime > 0)
    {
      machineState = 30;
    }
    if (currentTemp >= steamTempDetection)
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
      //after 35 seconds
      machineState = 35;
    }
    if (currentTemp >= steamTempDetection)
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
    if (currentTemp >= steamTempDetection)
    {
      machineState = 40;
    }
  case 40:
    if (currentTemp < steamTempDetection)
    {
      machineState = 45;
    }
    break;
  case 45:
    if (heatrateaverage > 0 && currentTemp < (gTargetTemp + 2))
    {
      machineState = 20;
    }
    if (currentTemp >= steamTempDetection)
    {
      machineState = 40;
    }
    break;
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
                 Serial.println("Start updating " + type);
               })
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
                   Serial.println("End Failed");
               });

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
}