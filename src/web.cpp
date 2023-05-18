#include<cstdlib>
#include <WiFi.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <sensor.h>
#include <heater.h>
#include <shared.h>
#include <config.h>
#include <ArduinoJson.h>

WiFiManager wifiManager;
AsyncWebServer server(80);

const int MAX_CONNECTION_RETRIES = 20;

const char* PARAM_MESSAGE = "message";
StaticJsonDocument<BUF_SIZE> json;

extern String local_ip;

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

double getDoublePostParameter(String parameterName, AsyncWebServerRequest *request){
    if(request->hasParam(parameterName, true)){
        AsyncWebParameter* p = request->getParam(parameterName, true);
        String newVal = p->value();
        double value = std::atof(newVal.c_str());
        return value;
    }
    return -1;
}

void handleUpdateConfig(AsyncWebServerRequest *request){
    bool updated = false;

    double temp = getDoublePostParameter("targetTemperature", request);
    if(temp > 0){
        updated = true;
        gTargetTemp = temp;
    }

    double n_P = getDoublePostParameter("P", request);
    if(n_P > 0){
        updated = true;
        gP = n_P;
    }

    double n_I = getDoublePostParameter("I", request);
    if(n_I > 0){
        updated = true;
        gI = n_I;
    }

    double n_D = getDoublePostParameter("D", request);
    if(n_D > 0){
        updated = true;
        gD = n_D;
    }

    double a_P = getDoublePostParameter("aP", request);
    if(a_P > 0){
        updated = true;
        gaP = a_P;
    }

    double a_I = getDoublePostParameter("aI", request);
    if(a_I > 0){
        updated = true;
        gaI = a_I;
    }

    double a_D = getDoublePostParameter("aD", request);
    if(a_D > 0){
        updated = true;
        gaD = a_D;
    }



    double overshoot = getDoublePostParameter("overshoot", request);
    if(overshoot > 0){
        updated = true;
        gOvershoot = overshoot;
    }

    if(updated){
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
    wifiManager.setConnectRetries(10);
    wifiManager.autoConnect("RancilioSilviaPID");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    local_ip = WiFi.localIP().toString();
    SPIFFS.begin();

    server.on("/status", HTTP_GET, handleStatus);
    server.on("/config", HTTP_GET, handleGetConfig);
    server.on("/config", HTTP_POST, handleUpdateConfig);

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.onNotFound(notFound);

    server.begin();
}