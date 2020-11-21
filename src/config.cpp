#include <ArduinoJson.h>
#include <shared.h>
#include "FS.h"
#include "SPIFFS.h"

bool prepareFS() {
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return false;
  }
  return true;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<BUF_SIZE> json;
  DeserializationError error = deserializeJson(json, buf.get());

  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  gTargetTemp = json["tset"];
  gOvershoot = json["tband"];
  gP = json["P"], gI = json["I"], gD = json["D"];
  gaP = json["aP"], gaI = json["aI"], gaD = json["aD"];

  return true;
}

bool saveConfig() {
  StaticJsonDocument<BUF_SIZE> json;
  json["tset"] = gTargetTemp;  json["tband"] = gOvershoot;
  json["P"] = gP, json["I"] = gI, json["D"] = gD;
  json["aP"] = gaP, json["aI"] = gaI, json["aD"] = gaD;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJson(json, configFile);
  return true;
}

void resetConfig() {
 gP=S_P; gI=S_I; gD=S_D;
 gaP=S_aP; gaI=S_aI; gaD=S_aD;
 gTargetTemp=S_TSET;
 gOvershoot=S_TBAND;
}