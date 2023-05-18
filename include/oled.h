void ssd1306_init ();
void ssd1306_display (float temperature_curr, float pressure_curr, double temperature_target, bool heater_state);
void ssd1306_draw_chart (float data, float* circuler_buffer, float min_input, float max_input, int chart_id, String data_str, String unit_str);
void drawStatusBar(String ip, double temperature_target, bool heater_state);
float readTempareture();
float readPressure();


