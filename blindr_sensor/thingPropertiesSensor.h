#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char SSID[] = SECRET_SSID; // Network SSID (name)
const char PASS[] = SECRET_PASS; // Network password (use for WPA, or use as key for WEP)
const char WEATHER_API_KEY[] = SECRET_WEATHER_API_KEY;

int current_light;
int current_rain;
int current_is_raining;
float current_temperature;

int threshold_light;
int threshold_rain;
float threshold_temperature;

int listener_light;
int listener_rain;
float listener_temperature;

String message;
String location;

void onThresholdLightChange();
void onThresholdRainChange();
void onThresholdTemperatureChange();

void onMessageChange();
void onLocationChange();

void initPropertiesSensor() {
  ArduinoCloud.addProperty(current_light, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(current_rain, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(current_temperature, READ, ON_CHANGE, NULL);

  ArduinoCloud.addProperty(threshold_light, READWRITE, ON_CHANGE, onThresholdLightChange);
  ArduinoCloud.addProperty(threshold_rain, READWRITE, ON_CHANGE, onThresholdRainChange);
  ArduinoCloud.addProperty(threshold_temperature, READWRITE, ON_CHANGE, onThresholdTemperatureChange);

  ArduinoCloud.addProperty(listener_light, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(listener_rain, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(listener_temperature, READ, ON_CHANGE, NULL);

  ArduinoCloud.addProperty(message, READWRITE, ON_CHANGE, onMessageChange);
  ArduinoCloud.addProperty(location, READWRITE, ON_CHANGE, onLocationChange);
}

WiFiConnectionHandler connection(SSID, PASS);
