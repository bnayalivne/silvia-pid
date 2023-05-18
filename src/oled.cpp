#ifndef SIMULATION_MODE
#include <Wire.h>
#include <oled.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===================== SSD1306  ======================================
// Constants for the SSD1306 display
// Constants for the SSD1306 display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Constants for the temperature and pressure charts
#define MAX_CHART_NUM 2 
#define MAX_DATA_POINTS 80
float temperatureData[MAX_DATA_POINTS];
float pressureData[MAX_DATA_POINTS];
int dataPointIndex[MAX_CHART_NUM];
float hight_per_chart = SCREEN_HEIGHT / 3;

// Constants for the temperature and pressure range
#define TEMPERATURE_MIN 0.0
#define TEMPERATURE_MAX 180.0
#define PRESSURE_MIN 0.0
#define PRESSURE_MAX 15.0

extern String local_ip;
String ip = "0.0.0.0";
// status bar
boolean _drawStatusBar = true; // change to show/hide status bar

// ===================== SSD1306  ======================================
void ssd1306_init () {
  // SSD1306
  // Initialize the SSD1306 display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

void ssd1306_display (float temperature_curr, float pressure_curr, double temperature_target, bool heater_state) {
  // Read temperature and pressure
  float temperature = temperature_curr; //readTempareture();
  float pressure = pressure_curr; // readPressure(); // Replace with your pressure reading code
  
  // Convert temperature and pressure to strings
  String temperatureString = String(temperature, 1) + (char)247;
  String pressureString = String(pressure, 1);
  String unitTempStr = "C";
  String unitPresStr = "bar";

  // Clear display
  display.clearDisplay();

  if(_drawStatusBar){
    ip = local_ip;
    drawStatusBar(ip, temperature_target, heater_state);
  }

  ssd1306_draw_chart(pressure, pressureData, PRESSURE_MIN, PRESSURE_MAX, 0, pressureString, unitPresStr);
  ssd1306_draw_chart(temperature, temperatureData, TEMPERATURE_MIN, TEMPERATURE_MAX, 1, temperatureString, unitTempStr);

  // Display the charts and text
  display.display();
}

void ssd1306_draw_chart (float data, float* circuler_buffer, float min_input, float max_input, int chart_id, String data_str, String unit_str) {
  int* ptr_point_index;

  ptr_point_index = &dataPointIndex[chart_id];

  // Update data
  circuler_buffer[*ptr_point_index] = data;
  *ptr_point_index = (*ptr_point_index + 1) % MAX_DATA_POINTS;

  // Draw temperature chart
  float min_value = min_input;
  float max_value = min_input;
  for (int i = 0; i < MAX_DATA_POINTS; i++) {
    float temp = circuler_buffer[i];
    if (temp < min_value) min_value = temp;
    if (temp > max_value) max_value = temp;
  }

  // Adjust min and max_value to fit within the desired range
  min_value = min_input; //max(min_value, min_input);
  max_value = max_input; //min(max_value, max_input);

  float data_range = max_value - min_value;
  float pixelsPerDegree = (float)hight_per_chart / data_range;
  for (int i = 0; i < MAX_DATA_POINTS - 1; i++) {
    float temp1 = circuler_buffer[(*ptr_point_index + i) % MAX_DATA_POINTS];
    float temp2 = circuler_buffer[(*ptr_point_index + i + 1) % MAX_DATA_POINTS];
    int y1 = SCREEN_HEIGHT - ((temp1 - min_value) * pixelsPerDegree) - 1 - 4 * chart_id - hight_per_chart * chart_id;
    int y2 = SCREEN_HEIGHT - ((temp2 - min_value) * pixelsPerDegree) - 1 - 4 * chart_id - hight_per_chart * chart_id;
    display.drawLine(i, y1, i + 1, y2, WHITE);
  }

  // Set text color and size
  display.setTextColor(WHITE);
  display.setTextSize(1);

  // Set text position for data
  int length_unit_str = unit_str.length();
  int text_X = MAX_DATA_POINTS + 3;
  int text_Y = SCREEN_HEIGHT - hight_per_chart * (chart_id + 1) + 10;
  display.setCursor(SCREEN_WIDTH - 6 * length_unit_str, text_Y);
  display.print(unit_str);
  display.setCursor(text_X, text_Y);
  display.print(data_str);
}

float readTempareture() {
  // Replace this function with your code to read the pressure from your sensor
  // and return the pressure value as a float

  return random(0, 180);
}

float readPressure() {
  // Replace this function with your code to read the pressure from your sensor
  // and return the pressure value as a float

  return random(0, 1);
}

/**
 * Draws the status bar at top of screen with fps and analog value
 */
void drawStatusBar(String ip, double temperature_target, bool heater_state) {
  
   // erase status bar by drawing all black
  display.fillRect(0, 0, display.width(), 16, SSD1306_WHITE); 
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);

  // Draw current val
  display.setCursor(0, 0);
  display.println("   Rancilio Silvia");

  // Draw current val
  display.setCursor(0, 8);
  ip = " " + ip;
  display.println(ip);

  display.setCursor(96, 8);
  String status;
  if (heater_state) status = "Heat";
  else              status = "Cool";
  display.println(status);

  display.setCursor(80, 16);
  String temperature_target_string = " " + String(temperature_target, 1) + (char)247 + "C ";
  display.println(temperature_target_string);

  display.setTextColor(SSD1306_WHITE);
}

#endif