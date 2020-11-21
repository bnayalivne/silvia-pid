#include<cstdlib>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <sensor.h>
#include <heater.h>
#include <shared.h>
#include <config.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

const char* ssid = "-----";
const char* password = "-----";

const char* PARAM_MESSAGE = "message";
StaticJsonDocument<BUF_SIZE> json;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void handleStatus(AsyncWebServerRequest *request) {
    String message = "{\"temperature\": ";
    message += currentTemp;
    message += ", \"targetTemperature\": ";
    message += gTargetTemp;
    message += ", \"boilerStatus\": ";
    message += heaterState ? "true" : "false";
    message += "}";
    request->send(200, "application/json", message);
}

void handleUpdateConfig(AsyncWebServerRequest *request){
    if(request->hasParam("targetTemperature", true)){
        AsyncWebParameter* p = request->getParam("targetTemperature", true);
        String newVal = p->value();
        double newTarget = std::atof(newVal.c_str());
        gTargetTemp = newTarget;
        saveConfig();
        request->send(200, "application/json", "{\"status\": true}");
    } else {
        request->send(400, "application/json", "{\"status\": false}");
    }
}

void handleGetConfig(AsyncWebServerRequest *request){
    StaticJsonDocument<BUF_SIZE> json;
    json["targetTemperature"] = gTargetTemp;  json["overshoot"] = gOvershoot;
    json["P"] = gP, json["I"] = gI, json["D"] = gD;
    json["aP"] = gaP, json["aI"] = gaI, json["aD"] = gaD;

    String output;
    serializeJson(json, output);
    request->send(200, "application/json", output);
}

void setupWeb() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    SPIFFS.begin();

    server.on("/status", HTTP_GET, handleStatus);
    server.on("/config", HTTP_GET, handleGetConfig);
    server.on("/config", HTTP_POST, handleUpdateConfig);

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.onNotFound(notFound);

    server.begin();
}