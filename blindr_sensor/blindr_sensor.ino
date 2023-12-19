#include "arduino_secrets_sensor.h"
#include "thingPropertiesSensor.h"
//#include <OneWire.h>
//#include <DallasTemperature.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>

// setup vars
const int PIN_LIGHT = A1;
const int PIN_RAIN = A0;
//const int PIN_TEMPERATURE = 16;

// loop controls
const int DELAY = 1000 * 2;
const int CYCLES_TEMPERATURE_READ = 60;

// default values
const int DEFAULT_THRESHOLD_LIGHT = 150;
const int DEFAULT_THRESHOLD_RAIN = 200;
const float DEFAULT_THRESHOLD_TEMPERATURE = 0.5;
const String DEFAULT_LOCATION = "Grenchen";

const char METHOD[] = "GET";
const char URL[] = "api.openweathermap.org";
const char PATH[] = "/data/2.5/weather";
const int PORT = 443;

const int MAX_ANALOG = 1023;
const float ZERO_KELVIN = -273.15;

char buffer[200];
int temperature_cycle_counter = 50;

int last_light;
int last_rain;
float last_temperature;
String current_location;

// temperature sensor
//OneWire oneWire(PIN_TEMPERATURE);
//DallasTemperature temperatureSensors(&oneWire);

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  delay(2000);

  initPropertiesSensor();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(connection);

  // default 0 (only errors)
  // max 4
  setDebugMessageLevel(4);
  ArduinoCloud.printDebugInfo();

  // temperature sensor
  //temperatureSensors.begin();

  // setup connection listeners
  connection.addCallback(NetworkConnectionEvent::CONNECTED, onNetworkConnect);
  connection.addCallback(NetworkConnectionEvent::DISCONNECTED, onNetworkDisconnect);
  connection.addCallback(NetworkConnectionEvent::ERROR, onNetworkError);
}

void loop() {
  // read values
  current_light = analogRead(PIN_LIGHT);
  current_rain = analogRead(PIN_RAIN);
  current_location = location.length() > 0 ? location : DEFAULT_LOCATION;

  // temperature sensor
  //temperatureSensors.requestTemperatures();
  //current_temperature = temperatureSensors.getTempCByIndex(0);

  // temperature api
  if (temperature_cycle_counter < CYCLES_TEMPERATURE_READ) temperature_cycle_counter++;
  else {
    temperature_cycle_counter = 0;
    update_temperature(current_location);
  }

  // prepare thresholds
  int current_threshold_light = threshold_light != 0 ? threshold_light : DEFAULT_THRESHOLD_LIGHT;
  int current_threshold_rain = threshold_rain != 0 ? threshold_rain : DEFAULT_THRESHOLD_RAIN;
  float current_threshold_temperature = threshold_temperature != 0.0 ? threshold_temperature : DEFAULT_THRESHOLD_TEMPERATURE;

  bool has_updates = false;
  // check if cloud variables should be updated
  if (abs(last_light - current_light) > current_threshold_light) {
    Serial.println("update light");
    last_light = current_light;
    // convert to % (0 dark -> 100 bright)
    listener_light = (float) current_light / MAX_ANALOG * 100;
    has_updates = true;
  }
  if (abs(last_rain - current_rain) > current_threshold_rain) {
    Serial.println("update rain");
    last_rain = current_rain;
    // invert and convert to % (0 dry -> 100 raining)
    listener_rain = (MAX_ANALOG - (float) current_rain) / MAX_ANALOG * 100;
    has_updates = true;
  }
  if (abs(last_temperature - current_temperature) > current_threshold_temperature) {
    Serial.println("update temperature");
    last_temperature = current_temperature;
    listener_temperature = current_temperature;
    has_updates = true;
  }

  // values
  sprintf(buffer, "current_light: %d current_rain: %d current_temperature: %.2f location: %s", current_light, current_rain, current_temperature, current_location.c_str());
  Serial.println(buffer);

  // thresholds
  sprintf(buffer, "t light: %d t rain: %d t temperature: %.2f", current_threshold_light, current_threshold_rain, current_threshold_temperature);
  Serial.println(buffer);

  if (has_updates) {
    Serial.println("has updates");
  }

  ArduinoCloud.update();

  busy_wait_us(DELAY * 1000);
}

/*
  temperature
*/
void update_temperature(String location) {
    Serial.println("get temperature");
    current_temperature = get_temperature(location);
    message = "temperature: " + location + " " + current_temperature + "°C";
}

float get_temperature(String location) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is not connectd!");
    return current_temperature;
  }

  // https://create.arduino.cc/projecthub/instanceofMA/fetch-the-easiest-way-to-make-http-requests-on-your-arduino-65bb24
  sprintf(buffer, "%s?q=%s&appid=%s", PATH, location.c_str(), WEATHER_API_KEY);
  String path = buffer;

  WiFiSSLClient client;

  while (!client.connect(URL, PORT)) {
    delay(500);
  }

  String request = 
      String(METHOD) + " " + path + " HTTP/1.1\r\n" +
      "Host: " + URL + "\r\n" +
      "Connection: close\r\n\r\n";

  client.print(request);

  // read headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  // read response
  String response;
  while (client.available()) {
    response += client.readStringUntil('\n');
  }

  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return current_temperature;
  }

  float temperature = doc["main"]["temp"];
  temperature += ZERO_KELVIN;
  Serial.print(location + ": ");
  Serial.println(temperature, 2);

  return temperature;
}

/*
  listeners
*/
void onThresholdLightChange() {

}

void onThresholdRainChange() {

}

void onThresholdTemperatureChange() {

}

void onMessageChange() {

}

void onLocationChange() {
  sprintf(buffer, "location changed to: %s", location);
  message = buffer;
  Serial.println(buffer);

  update_temperature(location);
}

/*
  network
*/
void onNetworkConnect() {
  Serial.println(">>>> SENSOR CONNECTED");
}

void onNetworkDisconnect() {
  Serial.println(">>>> SENSOR DISCONNECTED");
}

void onNetworkError() {
  Serial.println(">>>> SENSOR NETWORK ERROR");
}