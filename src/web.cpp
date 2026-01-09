#include <cstdlib>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <sensor.h>
#include <heater.h>
#include <shared.h>
#include <state_machine.h>
#include <config.h>
#include <ArduinoJson.h>

#define WIFI_CREDS_PATH "/wifi.json"

AsyncWebServer server(80);

const int MAX_CONNECTION_RETRIES = 20;

const char *PARAM_MESSAGE = "message";
StaticJsonDocument<BUF_SIZE> json;

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void handleStatus(AsyncWebServerRequest *request)
{

    String strMachineState = "Init";
    if (machineState == STATE_INIT || machineState == STATE_COLDSTART)
    {
        strMachineState = "Cold start";
    }
    else if (machineState == STATE_PREREADY || machineState == STATE_READY)
    {
        strMachineState = "Ready";
    }
    else if (machineState == STATE_BREWING || machineState == STATE_POSTBREW)
    {
        strMachineState = "Brewing";
    }
    else if (machineState == STATE_STEAM || machineState == STATE_CHILL)
    {
        strMachineState = "Steam";
    }

    String message = "{\"temperature\": ";
    message += currentTemp;
    message += ", \"targetTemperature\": ";
    message += gTargetTemp;
    message += ", \"time\": ";
    message += millis();
    message += ", \"boilerStatus\": ";
    message += heaterState ? "true" : "false";
    message += ", \"machineState\": \"";
    message += strMachineState;
    message += "\"}";
    request->send(200, "application/json", message);
}

double getDoublePostParameter(String parameterName, AsyncWebServerRequest *request)
{
    if (request->hasParam(parameterName, true))
    {
        const AsyncWebParameter *p = request->getParam(parameterName, true);
        String newVal = p->value();
        double value = std::atof(newVal.c_str());
        return value;
    }
    return -1;
}

void handleStartWifiSetup(AsyncWebServerRequest *request)
{
    // Remove stored station credentials and start AP for provisioning.
    // WiFi.disconnect(true) erases the WiFi config on some cores; call it to clear saved networks.
    WiFi.disconnect(true, true);
    delay(200);
    WiFi.mode(WIFI_OFF);
    delay(300);
    WiFi.mode(WIFI_AP);
    delay(200);
    bool ok = WiFi.softAP("Silvia-AP");
    if (!ok)
    {
        Serial.println("Failed to start softAP from /wifi-setup");
    }
    else 
    {
        IPAddress apIp = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(apIp);
    }
    request->send(200, "application/json", "{\"status\": true}");
}

// No blocking portal logic needed since WiFiManager has been removed.

void handleUpdateConfig(AsyncWebServerRequest *request)
{
    bool updated = false;

    double temp = getDoublePostParameter("targetTemperature", request);
    if (temp > 0)
    {
        updated = true;
        gTargetTemp = temp;
    }

    double n_P = getDoublePostParameter("P", request);
    if (n_P > 0)
    {
        updated = true;
        gP = n_P;
    }

    double n_I = getDoublePostParameter("I", request);
    if (n_I > 0)
    {
        updated = true;
        gI = n_I;
    }

    double n_D = getDoublePostParameter("D", request);
    if (n_D > 0)
    {
        updated = true;
        gD = n_D;
    }

    double a_P = getDoublePostParameter("aP", request);
    if (a_P > 0)
    {
        updated = true;
        gaP = a_P;
    }

    double a_I = getDoublePostParameter("aI", request);
    if (a_I > 0)
    {
        updated = true;
        gaI = a_I;
    }

    double a_D = getDoublePostParameter("aD", request);
    if (a_D > 0)
    {
        updated = true;
        gaD = a_D;
    }

    double overshoot = getDoublePostParameter("overshoot", request);
    if (overshoot > 0)
    {
        updated = true;
        gOvershoot = overshoot;
    }

    if (updated)
    {
        saveConfig();
        request->send(200, "application/json", "{\"status\": true}");
    }
    else
    {
        request->send(400, "application/json", "{\"status\": false}");
    }
}

void handleGetConfig(AsyncWebServerRequest *request)
{
    StaticJsonDocument<BUF_SIZE> json;
    json["targetTemperature"] = gTargetTemp;
    json["overshoot"] = gOvershoot;
    json["P"] = gP, json["I"] = gI, json["D"] = gD;
    json["aP"] = gaP, json["aI"] = gaI, json["aD"] = gaD;

    String output;
    serializeJson(json, output);
    request->send(200, "application/json", output);
}

void initWiFi() {
    WiFi.mode(WIFI_STA);
    delay(500);
    String ssid, password;
    bool credsLoaded = false;
    if (SPIFFS.exists(WIFI_CREDS_PATH)) {
        File f = SPIFFS.open(WIFI_CREDS_PATH, "r");
        if (f) {
            StaticJsonDocument<128> doc;
            DeserializationError err = deserializeJson(doc, f);
            if (!err) {
                ssid = doc["ssid"].as<String>();
                password = doc["password"].as<String>();
                credsLoaded = ssid.length() > 0;
            }
            f.close();
        }
    }
    bool wifiConnected = false;
    if (credsLoaded) {
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.print("Connecting to WiFi (stored creds): ");
        Serial.println(ssid);
        int connectionRetries = 0;
        while (WiFi.status() != WL_CONNECTED && connectionRetries < MAX_CONNECTION_RETRIES) {
            delay(500);
            Serial.print(".");
            connectionRetries++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            Serial.println("");
            Serial.println("WiFi connected.");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("");
            Serial.println("Failed to connect to WiFi with stored credentials.");
        }
    } else {
        Serial.println("No WiFi credentials found, not connecting.");
    }
    if (!wifiConnected) {
        // Fallback: start AP mode
        Serial.println("Starting fallback Access Point mode...");
        WiFi.disconnect(true, true);
        delay(200);
        WiFi.mode(WIFI_OFF);
        delay(300);
        WiFi.mode(WIFI_AP);
        delay(200);
        bool ok = WiFi.softAP("Silvia-AP");
        if (ok) {
            IPAddress apIp = WiFi.softAPIP();
            Serial.print("AP IP address: ");
            Serial.println(apIp);
        } else {
            Serial.println("Failed to start fallback AP mode");
        }
    }
}

void handleWifiCreds(AsyncWebServerRequest *request) {
    if (request->method() != HTTP_POST) {
        request->send(405, "application/json", "{\"error\":\"Method Not Allowed\"}");
        return;
    }
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing ssid or password\"}");
        return;
    }
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    StaticJsonDocument<128> doc;
    doc["ssid"] = ssid;
    doc["password"] = password;
    File f = SPIFFS.open(WIFI_CREDS_PATH, "w");
    if (!f) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
        return;
    }
    serializeJson(doc, f);
    f.close();
    request->send(200, "application/json", "{\"status\":true}");

    // Re-initialize WiFi with new credentials if possible
    initWiFi();
}

void setupWeb()
{
    initWiFi();
    SPIFFS.begin();

    server.on("/status", HTTP_GET, handleStatus);
    server.on("/config", HTTP_GET, handleGetConfig);
    server.on("/config", HTTP_POST, handleUpdateConfig);

    server.on("/wifi-creds", HTTP_POST, [](AsyncWebServerRequest *request){
        handleWifiCreds(request);
    });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.onNotFound(notFound);

    server.begin();
}