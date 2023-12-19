#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char SSID[] = SECRET_SSID; // Network SSID (name)
const char PASS[] = SECRET_PASS; // Network password (use for WPA, or use as key for WEP)

int light;
int rain;
float temperature;

float weight_light;
float weight_rain;
float weight_temperature;

int power;

String location;
String message_controller;

float listener_desired_state;
int listener_desired_direction;
float listener_desired_tilt;

float desired_state;
int desired_direction;
float desired_tilt;

float current_tilt;
int current_direction;
float current_state;

bool go_up;
bool go_down;

int threshold_light_down;
int threshold_light_tilt_close;
int threshold_light_tilt_open;
int threshold_light_up;

int threshold_rain_up;
int threshold_rain_tilt_close;

float threshold_temperature_down;
float threshold_temperature_tilt_close;
float threshold_temperature_tilt_open;
float threshold_temperature_up;

int threshold_power_up;
int threshold_power_down;

bool auto_mode;

// others
void updateDesiredState();

// listeners
void onLocationChange();
void onMessageChange();

void onDesiredStateChange();
void onDesiredDirectionChange();
void onDesiredTiltChange();

void onCurrentStateChange();
void onCurrentDirectionChange();
void onCurrentTiltChange();

void onGoUpChange();
void onGoDownChange();

void onAutoModeChange();

void onNetworkConnect();
void onNetworkDisconnect();
void onNetworkError();

void initPropertiesController() {

  ArduinoCloud.addProperty(light, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(rain, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(temperature, READWRITE, ON_CHANGE, updateDesiredState);

  ArduinoCloud.addProperty(weight_light, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(weight_rain, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(weight_temperature, READWRITE, ON_CHANGE, updateDesiredState);

  ArduinoCloud.addProperty(power, READ, ON_CHANGE, NULL);

  ArduinoCloud.addProperty(location, READWRITE, ON_CHANGE, onLocationChange);
  ArduinoCloud.addProperty(message_controller, READWRITE, ON_CHANGE, onMessageChange);

  ArduinoCloud.addProperty(listener_desired_state, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(listener_desired_direction, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(listener_desired_tilt, READ, ON_CHANGE, NULL);

  ArduinoCloud.addProperty(desired_state, READWRITE, ON_CHANGE, onDesiredStateChange);
  ArduinoCloud.addProperty(desired_direction, READWRITE, ON_CHANGE, onDesiredDirectionChange);
  ArduinoCloud.addProperty(desired_tilt, READWRITE, ON_CHANGE, onDesiredTiltChange);

  ArduinoCloud.addProperty(current_state, READWRITE, ON_CHANGE, onCurrentStateChange);
  ArduinoCloud.addProperty(current_direction, READWRITE, ON_CHANGE, onCurrentDirectionChange);
  ArduinoCloud.addProperty(current_tilt, READWRITE, ON_CHANGE, onCurrentTiltChange);

  ArduinoCloud.addProperty(go_up, READWRITE, ON_CHANGE, onGoUpChange);
  ArduinoCloud.addProperty(go_down, READWRITE, ON_CHANGE, onGoDownChange);


  ArduinoCloud.addProperty(threshold_light_down, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(threshold_light_tilt_close, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(threshold_light_tilt_open, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(threshold_light_up, READWRITE, ON_CHANGE, updateDesiredState);

  ArduinoCloud.addProperty(threshold_rain_up, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(threshold_rain_tilt_close, READWRITE, ON_CHANGE, updateDesiredState);

  ArduinoCloud.addProperty(threshold_temperature_down, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(threshold_temperature_tilt_close, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(threshold_temperature_tilt_open, READWRITE, ON_CHANGE, updateDesiredState);
  ArduinoCloud.addProperty(threshold_temperature_up, READWRITE, ON_CHANGE, updateDesiredState);

  ArduinoCloud.addProperty(threshold_power_up, READWRITE, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(threshold_power_down, READWRITE, ON_CHANGE, NULL);

  ArduinoCloud.addProperty(auto_mode, READWRITE, ON_CHANGE, onAutoModeChange);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
