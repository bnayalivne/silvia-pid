#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "screen.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

extern double currentTemp;
extern double gTargetTemp;
bool splashActive = true;
static unsigned long splashStart = 0;

void setupScreen()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }
}

void splashScreen()
{
    if (!splashActive)
        return;
    if (splashStart == 0)
    {
        splashStart = millis();
        IPAddress ip = WiFi.localIP();

        display.clearDisplay();
        display.drawBitmap(41, 6, splashFrame1, 48, 40, 1);

        display.setTextColor(1);
        display.setTextWrap(false);
        String ipStr = ip.toString();
        int16_t x1, y1;
        uint16_t w, h;
        display.setTextSize(1);
        display.getTextBounds(ipStr, 0, 0, &x1, &y1, &w, &h);
        int x = (SCREEN_WIDTH - w) / 2;
        display.setCursor(x, 53);
        display.print(ipStr);

        display.display();
    }
    if (millis() - splashStart > 5000)
    {
        splashActive = false;
    }
}

static unsigned long lastStatusUpdate = 0;
void statusScreen()
{
    unsigned long now = millis();
    if (now - lastStatusUpdate < 100)
    {
        return;
    }
    lastStatusUpdate = now;
    display.clearDisplay();
    int curTemp = (int)round(currentTemp);
    int tgtTemp = (int)round(gTargetTemp);
    double diff = fabs(currentTemp - gTargetTemp);
    static bool blinkState = true;
    static unsigned long lastBlink = 0;
    unsigned long nowBlink = millis();

    // show actual temp more time then hidden
    int blinkInterval = blinkState ? 1000 : 500;
    if (nowBlink - lastBlink > blinkInterval)
    {
        blinkState = !blinkState;
        lastBlink = nowBlink;
    }

    if (diff > 2.0)
    {
        // Show both temps, currentTemp is bigger and blinks, both centered
        if (blinkState)
        {
            display.setTextSize(3);
            String curStr = String(curTemp);
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(curStr, 0, 0, &x1, &y1, &w, &h);
            int x = (SCREEN_WIDTH - w) / 2;
            display.setCursor(x, 0);
            display.setTextColor(SSD1306_WHITE);
            display.print(curStr);
            display.setTextSize(2);
            display.print("c");
        }
        display.setTextSize(2);
        String tgtStr = String(tgtTemp);
        int16_t tx1, ty1;
        uint16_t tw, th;
        display.getTextBounds(tgtStr, 0, 0, &tx1, &ty1, &tw, &th);
        int tx = (SCREEN_WIDTH - tw) / 2;
        display.setCursor(tx, 38);
        display.setTextColor(SSD1306_WHITE);
        display.print(tgtStr);
        display.setTextSize(1);
        display.print("c");
    }
    else
    {
        // Only show currentTemp, no blink, big, centered
        display.setTextSize(3);
        String curStr = String(curTemp) + "C";
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(curStr, 0, 0, &x1, &y1, &w, &h);
        int x = (SCREEN_WIDTH - w) / 2;
        display.setCursor(x, 20);
        display.setTextColor(SSD1306_WHITE);
        display.print(curStr);
    }
    display.display();
}

void updateScreen()
{
    if (splashActive)
    {
        splashScreen();
    }
    else
    {
        statusScreen();
    }
}
