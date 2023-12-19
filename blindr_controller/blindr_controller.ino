#include "arduino_secrets_controller.h"
#include "thingPropertiesController.h"

const int PIN_POWER_VALUE = A1;
const int PIN_POWER = 2;
const int PIN_DIRECTION = 3;

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  delay(2000);

  // motor
  pinMode(PIN_POWER, OUTPUT);
  pinMode(PIN_DIRECTION, OUTPUT);

  digitalWrite(PIN_POWER, HIGH);
  digitalWrite(PIN_DIRECTION, HIGH);

  // Defined in thingProperties.h
  initPropertiesController();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // default 0 (only errors)
  // max 4
  setDebugMessageLevel(4);
  ArduinoCloud.printDebugInfo();

  // setup connection listeners
  ArduinoIoTPreferredConnection.addCallback(NetworkConnectionEvent::CONNECTED, onNetworkConnect);
  ArduinoIoTPreferredConnection.addCallback(NetworkConnectionEvent::DISCONNECTED, onNetworkDisconnect);
  ArduinoIoTPreferredConnection.addCallback(NetworkConnectionEvent::ERROR, onNetworkError);
}

// loop controls
const int DELAY = 200;
const int DELAY_POWER_ON = 100;
const int DELAY_POWER_OFF = 100;
const int DELAY_RAMP_UP = 75;

// vars
const float STATE_UP = 0.0;
const float STATE_DOWN = 1.0;
const float THRESHOLD_STATE = 0.010;

const float STATE_TILT_CLOSE = 0.0;
const float STATE_TILT_OPEN = 1.0;
const float THRESHOLD_TILT = 0.010;

const int POWER_ON = LOW;
const int POWER_OFF = HIGH;

const int DIRECTION_UP = LOW;
const int DIRECTION_DOWN = HIGH;

const int THRESHOLD_RAMP_UP_POWER = 950;

const int POWER_READ_INTERVAL = 150;
const int RAMP_UP_INTERVAL = 100;

const float STATE_MULTIPLIER_SECOND = 0.010;

int ramp_up_power = 0;
int threshold_power = 0;

char buffer[100];

// interval controls
// direction
const unsigned long INTERVAL_DIRECTION_CHECK = 150;
unsigned long timestamp_direction_check = 0;

// state
const unsigned long INTERVAL_STATE_CHECK = 100;
unsigned long timestamp_state_check = 0;

// ongoing movement
unsigned long timestamp_state_change = 0;
float initial_state = 0.0;

// auto mode
String message = "";

/*
  motor
*/
int getPowerValue() {
  int total = 0;
  // sum up multiple reads
  for (int i = 0; i < POWER_READ_INTERVAL; i++) {
    total += analogRead(PIN_POWER_VALUE);
  }
  // return average to remove noise etc.
  return total / POWER_READ_INTERVAL;
}

int getCurrentReachedState() {
  return current_direction == DIRECTION_UP ? STATE_UP : STATE_DOWN;
}

bool powerOn() {
  delay(DELAY_POWER_ON);
  digitalWrite(PIN_POWER, POWER_ON);
  delay(DELAY_RAMP_UP);
  // ramp up motor
  for (int i = 0; i < RAMP_UP_INTERVAL; i++) {
    ramp_up_power = getPowerValue();
    if (ramp_up_power > THRESHOLD_RAMP_UP_POWER) {
      powerOff();
      // update current state on resistence
      desired_state = current_state = getCurrentReachedState();
      sprintf(buffer, ">>>> RAMP UP POWER OFF GOING %s: %d > %d", current_direction == DIRECTION_UP ? "UP" : "DOWN", ramp_up_power, THRESHOLD_RAMP_UP_POWER);
      Serial.println(buffer);
      message_controller = buffer;
      // could not power on
      return false;
    }
    // do nothing
  }
  // could power on
  return true;
}

void powerOff() {
  digitalWrite(PIN_POWER, POWER_OFF);
  digitalWrite(PIN_DIRECTION, DIRECTION_DOWN);
  delay(DELAY_POWER_OFF);
}

int getPowerThreshold() {
  return current_direction == DIRECTION_UP ? threshold_power_up : threshold_power_down;
}

/*
  loop
*/
void loop() {
  // always update cloud
  ArduinoCloud.update();

  // check for resistence
  power = getPowerValue();
  threshold_power = getPowerThreshold();  
  if (power > threshold_power) {
    powerOff();
    // update current state on resistence
    desired_state = current_state = getCurrentReachedState();
    sprintf(buffer, ">>>> POWER OFF GOING %s: %d > %d", current_direction == DIRECTION_UP ? "UP" : "DOWN", power, threshold_power);
    Serial.println(buffer);
    message_controller = buffer;
    return;
  }

  // check if needs to close gap
  if (hasStateDifference() && powerOn()) {
    updateCurrentState();
  } else {
    powerOff();
  }
}

bool hasStateDifference() {
  if (current_direction == DIRECTION_UP) {
    return current_state > desired_state;
  }
  if (current_direction == DIRECTION_DOWN) {
    return current_state < desired_state;
  }
  return true;
}

void updateCurrentState() {
  float difference = (millis() - timestamp_state_change) / 1000.0 * STATE_MULTIPLIER_SECOND;

  if (current_direction == DIRECTION_UP) {
    current_state = initial_state - difference;
    // min 0.0
    if (current_state < STATE_UP) current_state = STATE_UP;
  }

  if (current_direction == DIRECTION_DOWN) {
    current_state = initial_state + difference;
    // max 1.0
    if (current_state > STATE_DOWN) current_state = STATE_DOWN;
  }
}

bool hasDirectionUpdate() {
  // check interval
  // if (millis() - timestamp_direction_check < INTERVAL_DIRECTION_CHECK) return false;

  return current_direction != desired_direction;  
}

bool hasStateUpdate() {
  // check interval
  // if (millis() - timestamp_state_check < INTERVAL_STATE_CHECK) return false;

  return current_state != desired_state;
}

void updateDesiredState() {
  // skip calculating desired state when in auto mode
  if (!auto_mode) {
    Serial.println(buffer);
    message_controller = "skip desired state update because the auto mode is off";
    return;
  }

  float down = 0.0;
  float tilt_close = 0.0;
  float tilt_open = 0.0;
  float up = 0.0;

  // light
  if (light > threshold_light_down) down += weight_light;
  if (light > threshold_light_tilt_close) tilt_close += weight_light;
  if (light < threshold_light_tilt_open) tilt_open += weight_light;
  if (light < threshold_light_up) up += weight_light;

  // rain
  if (rain > threshold_rain_up) up += weight_rain;
  if (rain > threshold_rain_tilt_close) tilt_close += weight_rain;

  // temperature
  if (temperature > threshold_temperature_down) down += weight_temperature;
  if (temperature > threshold_temperature_tilt_close) tilt_close += weight_temperature;
  if (temperature < threshold_temperature_tilt_open) tilt_open += weight_temperature;
  if (temperature < threshold_temperature_up) up += weight_temperature;

  //sprintf(buffer, "up: %.2f down: %.2f tilt_close: %.2f tilt_open: %.2f", up, down, tilt_close, tilt_open);
  sprintf(buffer, "up: %.2f down: %.2f", up, down);
  Serial.println(buffer);
  message_controller = buffer;

  if (up > down) {
    // go up
    message = "auto mode: go up";
    listener_desired_state = STATE_UP;
  } else {
    // go down
    message = "auto mode: go down";
    listener_desired_state = STATE_DOWN;
    
    if (tilt_open > tilt_close) {
      // tilt open
      //message += "\nauto mode: tilt open";
      listener_desired_tilt = STATE_TILT_OPEN;
    } else {
      // tilt close
      //message += "\nauto mode: tilt close";
      listener_desired_tilt = STATE_TILT_CLOSE;
    }
  }
  Serial.println(message);
  message_controller = message;
}

/*
  listeners
*/
void onLocationChange() {
  sprintf(buffer, "location: %s", location.c_str());
  Serial.println(buffer);
}

void onMessageChange() {
  sprintf(buffer, "message: %s", message_controller.c_str());
  Serial.println(buffer);  
}

void onDesiredStateChange() {
  sprintf(buffer, "desired_state: %.2f", desired_state);
  Serial.println(buffer);

  if (abs(desired_state - current_state) > THRESHOLD_STATE) {
    // update desired state
    desired_direction = desired_state <= current_state ? DIRECTION_UP : DIRECTION_DOWN;

    // check for direction change
    if (current_direction != desired_direction) {
      powerOff();
      current_direction = desired_direction;

      sprintf(buffer, "direction update: GO %s", current_direction == DIRECTION_UP ? "UP" : "DOWN");
      Serial.println(buffer);
      message_controller = buffer;
    }

    // reset current state
    if (desired_state == STATE_UP) {
      current_state = STATE_DOWN;
    }
    if (desired_state == STATE_DOWN) {
      current_state = STATE_UP;
    }
    
    // set direction
    digitalWrite(PIN_DIRECTION, current_direction);

    // save timestamp to calculate seconds of moving in direction
    timestamp_state_change = millis();
    initial_state = current_state;
  }
}

void onDesiredDirectionChange() {
  sprintf(buffer, "desired_direction: %.2f", desired_direction);
  Serial.println(buffer);
}

void onDesiredTiltChange() {
  sprintf(buffer, "desired_tilt: %.2f", desired_tilt);
  Serial.println(buffer);

  if (abs(desired_tilt - current_tilt) > THRESHOLD_STATE) {
    // TODO: ?
  }
}

void onCurrentStateChange() {
  sprintf(buffer, "current_state: %.2f", current_state);
  Serial.println(buffer);
}

void onCurrentDirectionChange() {
  sprintf(buffer, "current_direction: %s", current_direction == DIRECTION_UP ? "UP" : "DOWN");
  Serial.println(buffer);
}

void onCurrentTiltChange() {
  sprintf(buffer, "current_tilt: %.2f", current_tilt);
  Serial.println(buffer);
}

void onGoUpChange() {
  Serial.println("button go up");
  listener_desired_state = STATE_UP;
}

void onGoDownChange() {
  Serial.println("button go down");
  listener_desired_state = STATE_DOWN;
}

void onAutoModeChange() {
  sprintf(buffer, "auto_mode: %s", auto_mode ? "ON" : "OFF");
  Serial.println(buffer);
  message_controller = buffer;

  updateDesiredState();
}

/*
  network
*/
void onNetworkConnect() {
  Serial.println(">>>> CONTROLLER CONNECTED");
}

void onNetworkDisconnect() {
  Serial.println(">>>> CONTROLLER DISCONNECTED");
}

void onNetworkError() {
  Serial.println(">>>> CONTROLLER NETWORK ERROR");
}