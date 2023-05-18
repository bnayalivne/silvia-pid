#include <Arduino.h>
#include <web.h>
#include <shared.h>
#include <sensor.h>
#include <heater.h>
#include <PID_v1.h>
#include <oled.h>
#include <config.h>

#define PID_INTERVAL 200
//
// STANDARD reset values based on Silvia PID
//

double currentTemp = 0;
double currentPres = 0;

double gTargetTemp=S_TSET;
double gOvershoot=S_TBAND;
double gOutputPwr=0.0;
double gP = S_P, gI = S_I, gD = S_D;
double gaP = S_aP, gaI = S_aI, gaD = S_aD;

boolean overShootMode = false;
unsigned long time_now=0;
unsigned long time_last=0;

PID ESPPID(&currentTemp, &gOutputPwr, &gTargetTemp, gP, gI, gD, DIRECT);

String local_ip;

void setup() {
  Serial.begin(115200);

  Serial.println("Mounting SPIFFS...");
  if(!prepareFS()) {
    Serial.println("Failed to mount SPIFFS !");
  } else {
    Serial.println("Mounted.");
  }
  Serial.println("Loading config...");
  if (!loadConfig()) {
    Serial.println("Failed to load config. Using default values and creating config...");
    if (!saveConfig()) {
     Serial.println("Failed to save config");
    } else {
      Serial.println("Config saved");
    }
  } else {
    Serial.println("Config loaded");
  }

  setupWeb();
  setupSensor();
  setupHeater();
  ssd1306_init();

  // start PID
  ESPPID.SetTunings(gP, gI, gD);
  ESPPID.SetSampleTime(PID_INTERVAL);
  ESPPID.SetOutputLimits(0, 1000);
  ESPPID.SetMode(AUTOMATIC);

  time_now=millis();
  time_last=time_now;
}

void loop() {
  time_now=millis();
  currentTemp = getCurrentTemperature();

  if(abs((double)(time_now-time_last))>=PID_INTERVAL or time_last > time_now) {
    if( !overShootMode && abs((double)(gTargetTemp-currentTemp))>=gOvershoot ) {        
      ESPPID.SetTunings(gaP, gaI, gaD);
      overShootMode=true;
    }
    else if( overShootMode && abs((double)(gTargetTemp-currentTemp))<gOvershoot ) {
      ESPPID.SetTunings(gP,gI,gD);
      overShootMode=false;
    }
    if(ESPPID.Compute()==true) {   
      setHeatPowerPercentage(gOutputPwr);
    }
    time_last=time_now;
  }

  ssd1306_display(currentTemp, currentPres, gTargetTemp, heaterState);
  updateHeater();
}