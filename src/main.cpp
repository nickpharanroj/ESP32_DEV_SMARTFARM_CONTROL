#include <Arduino.h>
#include "DevIsoInput.h"
#include "DevRelay.h"
#include "DevSwitch.h"

// Pin definitions (from HardwareESP32Config.md)
// --- Switch ---
const uint8_t PIN_SW1 = 34; // SW1 = Enter/Select (Active Low, External R10k)
const uint8_t PIN_SW2 = 35; // SW2 = Down         (Active Low, External R10k)
const uint8_t PIN_SW3 = 32; // SW3 = Up            (Active Low, External R10k)

// --- Relay ---
const uint8_t PIN_RELAY1 = 4;  // Relay1 = Fan    (Active Low)
const uint8_t PIN_RELAY2 = 16; // Relay2 = Pump   (Active Low)
const uint8_t PIN_RELAY3 = 17; // Relay3 = Heater (Active Low)

// --- Isolated Input ---
const uint8_t PIN_ISO1 = 33; // ISO1 = TankLevelSensor1 (น้ำแห้ง) (Active Low)
const uint8_t PIN_ISO2 = 27; // ISO2 = TankLevelSensor2 (น้ำล้น)  (Active Low)

// Instantiate switches (Active Low)
DevSwitch sw1(PIN_SW1, false);
DevSwitch sw2(PIN_SW2, false);
DevSwitch sw3(PIN_SW3, false);

// Instantiate relays (Active Low)
DevRelay relayFan   (PIN_RELAY1); // Relay1 - Fan
DevRelay relayPump  (PIN_RELAY2); // Relay2 - Pump
DevRelay relayHeater(PIN_RELAY3); // Relay3 - Heater

// Instantiate isolated inputs (Active Low)
DevIsoInput tankLevelLow (PIN_ISO1, false);  // ISO1 - TankLevelSensor1 (น้ำแห้ง)
DevIsoInput tankLevelHigh(PIN_ISO2, false);  // ISO2 - TankLevelSensor2 (น้ำล้น)

// Callback handlers
void onSw1Click() {
  Serial.println("SW1: Enter/Select");
}

void onSw2Click() {
  Serial.println("SW2: Down");
}

void onSw3Click() {
  Serial.println("SW3: Up");
}

// Tank level sensor callbacks
void onTankLowActive() {
  Serial.println("[ALERT] Tank Level LOW - น้ำแห้ง!");
}

void onTankLowInactive() {
  Serial.println("[INFO] Tank Level LOW cleared - น้ำกลับมาปกติ");
}

void onTankHighActive() {
  Serial.println("[ALERT] Tank Level HIGH - น้ำล้น!");
}

void onTankHighInactive() {
  Serial.println("[INFO] Tank Level HIGH cleared - น้ำลดลงปกติ");
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // Initialize switches
  sw1.begin();
  sw2.begin();
  sw3.begin();

  // Register click callbacks
  sw1.onClick(onSw1Click);
  sw2.onClick(onSw2Click);
  sw3.onClick(onSw3Click);

  // Initialize relays (all start OFF)
  relayFan.begin();
  relayPump.begin();
  relayHeater.begin();

  // Initialize isolated inputs (tank level sensors)
  tankLevelLow.begin();
  tankLevelHigh.begin();

  // Register tank level sensor callbacks
  tankLevelLow.onActive(onTankLowActive);
  tankLevelLow.onInactive(onTankLowInactive);
  tankLevelHigh.onActive(onTankHighActive);
  tankLevelHigh.onInactive(onTankHighInactive);

  Serial.println("System ready.");
}

void handleSerial() {
  if (!Serial.available()) return;

  char cmd = Serial.read();
  switch (cmd) {
    case 'f': case 'F':
      relayFan.toggle();
      Serial.printf("Fan   -> %s\n", relayFan.getState() ? "ON" : "OFF");
      break;
    case 'p': case 'P':
      relayPump.toggle();
      Serial.printf("Pump  -> %s\n", relayPump.getState() ? "ON" : "OFF");
      break;
    case 'h': case 'H':
      relayHeater.toggle();
      Serial.printf("Heater-> %s\n", relayHeater.getState() ? "ON" : "OFF");
      break;
  }
}

void loop() {
  // Poll switches (debounce and edge detection handled by class)
  sw1.update();
  sw2.update();
  sw3.update();

  // Poll tank level sensors
  tankLevelLow.update();
  tankLevelHigh.update();

  handleSerial();

  delay(10);
}