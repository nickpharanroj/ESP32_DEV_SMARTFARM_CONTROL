#include <Arduino.h>
#include "DevIsoInput.h"
#include "DevRelay.h"
#include "DevSwitch.h"

// OLED display library
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Instantiate OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions (from HardwareESP32Config.md)
const uint8_t PIN_SW1 = 34; // SW1 = Enter/Select (Active Low)
const uint8_t PIN_SW2 = 35; // SW2 = Down (Active Low)
const uint8_t PIN_SW3 = 32; // SW3 = Up (Active Low)

// Instantiate switches (Active Low)
DevSwitch sw1(PIN_SW1, false);
DevSwitch sw2(PIN_SW2, false);
DevSwitch sw3(PIN_SW3, false);

// Relay pin definitions (from HardwareESP32Config.md)
const uint8_t PIN_RELAY1 = 4;  // Relay1 = Fan (Active Low)
const uint8_t PIN_RELAY2 = 16; // Relay2 = Pump (Active Low)
const uint8_t PIN_RELAY3 = 17; // Relay3 = Heater (Active Low)

// Instantiate relays (Active Low)
DevRelayWithTimer relayFan(PIN_RELAY1, true);
DevRelayWithTimer relayPump(PIN_RELAY2, true);
DevRelayWithTimer relayHeater(PIN_RELAY3, true);

// Isolated inputs (from HardwareESP32Config.md)
const uint8_t PIN_ISO1 = 33; // ISO1 = TankLevelSensor1 (water dry) Active Low
const uint8_t PIN_ISO2 = 27; // ISO2 = TankLevelSensor2 (water overflow) Active Low

// Instantiate isolated inputs (Active Low)
DevIsoInput iso1(PIN_ISO1, false);
DevIsoInput iso2(PIN_ISO2, false);

// Display update timing
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 250; // ms

// Forward declarations
void showWelcome();
void updateDisplay();

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

// Relay control helpers
void toggleFan() {
  relayFan.toggle();
  Serial.printf("Fan: %s\n", relayFan.getState() ? "ON" : "OFF");
}

void togglePump() {
  relayPump.toggle();
  Serial.printf("Pump: %s\n", relayPump.getState() ? "ON" : "OFF");
}

void toggleHeater() {
  relayHeater.toggle();
  Serial.printf("Heater: %s\n", relayHeater.getState() ? "ON" : "OFF");
}

// ISO callbacks
void onIso1Active() {
  Serial.println("ISO1: TankLevelSensor1 - DRY (active)");
}

void onIso1Inactive() {
  Serial.println("ISO1: TankLevelSensor1 - OK (inactive)");
}

void onIso2Active() {
  Serial.println("ISO2: TankLevelSensor2 - OVERFLOW (active)");
}

void onIso2Inactive() {
  Serial.println("ISO2: TankLevelSensor2 - OK (inactive)");
}

// --- OLED helpers ---
void showWelcome() {
  display.clearDisplay();
  
  // Draw frame
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.drawRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2, SSD1306_WHITE);
  
  // Title
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(8, 12);
  display.println("ESP32");
  display.setCursor(8, 28);
  display.println("DevKit V2");
  
  // Subtitle
  display.setTextSize(1);
  display.setCursor(10, 48);
  display.println("Smart Farm Control");
  
  display.display();
  delay(2000);
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setCursor(12, 0);
  display.println("ESP32 Smart Farm");
  display.drawFastHLine(0, 9, SCREEN_WIDTH, SSD1306_WHITE);

  // Column headers
  display.setCursor(0, 13);
  display.print("SWITCH:");
  display.setCursor(72, 13);
  display.print("RELAY:");

  // Left column - Switches (use checkboxes style)
  display.setCursor(2, 23);
  display.print("Up  ["); display.print(sw3.isPressed() ? "X" : " "); display.print("]");
  display.setCursor(2, 33);
  display.print("Dn  ["); display.print(sw2.isPressed() ? "X" : " "); display.print("]");
  display.setCursor(2, 43);
  display.print("Sel ["); display.print(sw1.isPressed() ? "X" : " "); display.print("]");

  // Vertical divider
  display.drawFastVLine(64, 13, 40, SSD1306_WHITE);

  // Right column - Relays
  display.setCursor(68, 23);
  display.print("Fan ["); display.print(relayFan.getState() ? "X" : " "); display.print("]");
  display.setCursor(68, 33);
  display.print("Pmp ["); display.print(relayPump.getState() ? "X" : " "); display.print("]");
  display.setCursor(68, 43);
  display.print("Htr ["); display.print(relayHeater.getState() ? "X" : " "); display.print("]");

  // Bottom separator
  display.drawFastHLine(0, 53, SCREEN_WIDTH, SSD1306_WHITE);

  // Tank status at bottom
  display.setCursor(8, 56);
  display.print("TANK: T1[");
  display.print(iso1.isActive() ? "DRY" : "OK");
  display.print("] T2[");
  display.print(iso2.isActive() ? "FUL" : "OK");
  display.print("]");

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
  } else {
    showWelcome();
  }

  // Initialize switches
  sw1.begin();
  sw2.begin();
  sw3.begin();

  // Initialize relays
  relayFan.begin();
  relayPump.begin();
  relayHeater.begin();

  // Initialize isolated inputs
  iso1.begin();
  iso2.begin();

  // Register ISO callbacks
  iso1.onActive(onIso1Active);
  iso1.onInactive(onIso1Inactive);
  iso2.onActive(onIso2Active);
  iso2.onInactive(onIso2Inactive);

  // Register click callbacks
  sw1.onClick(onSw1Click);
  sw2.onClick(onSw2Click);
  sw3.onClick(onSw3Click);
}

void loop() {
  // Poll switches (debounce and edge detection handled by class)
  sw1.update();
  sw2.update();
  sw3.update();

  // Poll isolated inputs
  iso1.update();
  iso2.update();

  // Serial control: press keys to toggle relays
  if (Serial.available()) {
    char c = (char)Serial.read();
    switch (c) {
      case 'f': case 'F': toggleFan(); break;
      case 'p': case 'P': togglePump(); break;
      case 'h': case 'H': toggleHeater(); break;
      case '1': toggleFan(); break;
      case '2': togglePump(); break;
      case '3': toggleHeater(); break;
      default: break;
    }
  }

  delay(10);

  // Update display at interval
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
}