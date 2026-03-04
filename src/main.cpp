#include <Arduino.h>
#include "DevIsoInput.h"
#include "DevRelay.h"
#include "DevSwitch.h"

// OLED display library
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi Manager
#include <WiFi.h>
#include <WiFiManager.h>

// OLED display configuration
#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 128
#endif
#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 64
#endif
#ifndef OLED_RESET
#define OLED_RESET -1
#endif
#ifndef SCREEN_ADDRESS
#define SCREEN_ADDRESS 0x3C
#endif

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

// WiFi variables
String ipAddress = "Not Connected";
bool wifiConnected = false;

// WiFi Reset detection
const unsigned long WIFI_RESET_HOLD_TIME = 5000; // 5 seconds
unsigned long sw1PressStart = 0;
bool sw1LongPressHandled = false;

// Forward declarations
void showWelcome();
void updateDisplay();
void showCountdown(int seconds);
void setupWiFi();
void checkWiFiResetButton();

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
  display.setCursor(8, 8);
  display.print("ESP32");
  display.setCursor(8, 24);
  display.print("DevKit V2");
  
  // Subtitle
  display.setTextSize(1);
  display.setCursor(10, 43);
  display.print("Smart Farm Control");
  
  // WiFi Status
  display.setTextSize(1);
  if (wifiConnected) {
    // Show IP Address (center align bottom)
    display.setCursor(6, 56);
    display.print("IP:");
    display.print(ipAddress);
  } else {
    // Show SSID for config
    display.setCursor(12, 56);
    display.print("SSID:ESP32_Farm");
  }
  
  display.display();
  delay(3000);
}

void showCountdown(int seconds) {
  display.clearDisplay();
  
  // Border
  display.drawRect(10, 10, SCREEN_WIDTH-20, SCREEN_HEIGHT-20, SSD1306_WHITE);
  
  // Warning message
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 16);
  display.println("WiFi Reset in:");
  
  // Countdown number (large)
  display.setTextSize(3);
  display.setCursor(52, 32);
  display.println(seconds);
  
  display.display();
}

void setupWiFi() {
  display.clearDisplay();
  
  // Draw border
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  
  // Title
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 8);
  display.print("WiFi Setup Mode");
  display.drawFastHLine(4, 18, SCREEN_WIDTH-8, SSD1306_WHITE);
  
  // Instructions
  display.setCursor(6, 24);
  display.print("Connect to WiFi:");
  
  // SSID (highlighted)
  display.setTextSize(2);
  display.setCursor(6, 36);
  display.print("ESP32_Farm");
  
  // Bottom instruction
  display.setTextSize(1);
  display.setCursor(4, 54);
  display.print("IP:192.168.4.1");
  
  display.display();
  
  Serial.println("Starting WiFi Manager...");
  Serial.println("SSID: ESP32_Farm");
  Serial.println("IP: 192.168.4.1");
  
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180); // 3 minutes timeout
  
  // Try to connect
  if (wifiManager.autoConnect("ESP32_Farm")) {
    wifiConnected = true;
    ipAddress = WiFi.localIP().toString();
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(ipAddress);
    
    // Show success message
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(20, 24);
    display.print("WiFi Connected!");
    display.setCursor(6, 36);
    display.print("IP:");
    display.print(ipAddress);
    display.display();
    delay(2000);
  } else {
    wifiConnected = false;
    ipAddress = "Not Connected";
    Serial.println("WiFi connection failed");
  }
}

// Check for WiFi reset button during startup
void checkWiFiResetButton() {
  // Initialize SW1 for reset check
  pinMode(PIN_SW1, INPUT_PULLUP);
  
  // Check if SW1 is pressed at startup
  bool sw1Pressed = (digitalRead(PIN_SW1) == LOW); // Active Low
  
  if (sw1Pressed) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(8, 20);
    display.print("Hold for WiFi Reset");
    display.display();
    
    unsigned long pressStart = millis();
    bool resetTriggered = false;
    
    // Wait and check if button held for 5 seconds
    while ((millis() - pressStart) < WIFI_RESET_HOLD_TIME) {
      // Check if button is still pressed
      if (digitalRead(PIN_SW1) != LOW) {
        // Button released
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(20, 28);
        display.print("Reset Cancelled");
        display.display();
        delay(1000);
        return;
      }
      
      // Show countdown
      unsigned long elapsed = millis() - pressStart;
      if (elapsed >= 1000) {
        int remainingSeconds = 5 - (elapsed / 1000);
        if (remainingSeconds >= 0) {
          showCountdown(remainingSeconds);
        }
      }
      
      delay(100);
    }
    
    // Button held for 5 seconds - reset WiFi
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(8, 24);
    display.print("Resetting WiFi...");
    display.display();
    
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(1000);
    
    display.clearDisplay();
    display.setCursor(8, 24);
    display.print("WiFi Reset!");
    display.setCursor(8, 36);
    display.print("Rebooting...");
    display.display();
    delay(2000);
    
    ESP.restart();
  }
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
  }
  
  // Check for WiFi reset button (SW1) at startup
  checkWiFiResetButton();
  
  // Setup WiFi
  setupWiFi();
  
  // Show welcome with IP
  showWelcome();

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
