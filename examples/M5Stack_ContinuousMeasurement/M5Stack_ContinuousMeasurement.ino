/**
 * @file M5Stack_ContinuousMeasurement.ino
 * @brief Continuous distance measurement display on M5Stack Core2
 *
 * This example demonstrates continuous distance measurement using the
 * SEN0366 Laser Rangefinder connected to M5Stack Core2 Port A (G32/G33).
 *
 * Hardware Setup:
 * - M5Stack Core2
 * - SEN0366 Laser Rangefinder connected to Port A
 *   - TX (Rangefinder) -> G33 (RX on M5Stack)
 *   - RX (Rangefinder) -> G32 (TX on M5Stack)
 *   - VCC -> 5V
 *   - GND -> GND
 *
 * Features:
 * - Large, easy-to-read distance display
 * - Real-time measurement updates
 * - Beautiful font rendering
 * - Status indicators
 *
 * @author GJD-01
 * @date 2024
 */

#include <M5Unified.h>
#include <LaserRangefinder_SEN0366.h>

// Port A pins on M5Stack Core2
#define RXD2 32  // G33
#define TXD2 33  // G32

// Create rangefinder object using Serial2 (Port A)
LaserRangefinder_SEN0366 rangefinder(&Serial2);

// Display parameters
const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;
const uint16_t COLOR_BG = TFT_BLACK;
const uint16_t COLOR_TEXT = TFT_WHITE;
const uint16_t COLOR_DISTANCE = TFT_CYAN;
const uint16_t COLOR_UNIT = TFT_YELLOW;
const uint16_t COLOR_STATUS_OK = TFT_GREEN;
const uint16_t COLOR_STATUS_ERROR = TFT_RED;

// Measurement state
float currentDistance = 0.0;
bool measurementActive = false;
unsigned long lastUpdateTime = 0;
unsigned long lastMeasurementTime = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 100; // ms
int measurementCount = 0;
int errorCount = 0;

void setup() {
    // Initialize M5Stack
    auto cfg = M5.config();
    M5.begin(cfg);

    // Initialize display
    M5.Display.setRotation(1);
    M5.Display.fillScreen(COLOR_BG);
    M5.Display.setTextDatum(middle_center);

    // Show startup screen
    displayStartupScreen();
    delay(2000);

    // Initialize rangefinder
    Serial.begin(115200);
    Serial.println("Initializing SEN0366 Laser Rangefinder...");

    rangefinder.begin(9600);
    delay(500);

    // Configure rangefinder for optimal continuous measurement
    Serial.println("Configuring rangefinder...");

    // Set resolution to 0.1mm for better accuracy
    if (rangefinder.setResolution(SEN0366_RESOLUTION_01MM)) {
        Serial.println("Resolution set to 0.1mm");
    } else {
        Serial.println("Warning: Could not set resolution");
    }

    delay(100);

    // Set measurement frequency to 10Hz
    if (rangefinder.setFrequency(SEN0366_FREQ_10HZ)) {
        Serial.println("Frequency set to 10Hz");
    } else {
        Serial.println("Warning: Could not set frequency");
    }

    delay(100);

    // Start continuous measurement
    Serial.println("Starting continuous measurement...");
    if (rangefinder.startContinuousMeasurement()) {
        measurementActive = true;
        Serial.println("Continuous measurement started");
    } else {
        Serial.println("ERROR: Could not start continuous measurement");
        displayError("Failed to start measurement");
        while (1) delay(100);
    }

    // Draw main screen
    drawMainScreen();
}

void loop() {
    M5.update();

    // Check for button press to stop/start measurement
    if (M5.BtnA.wasPressed()) {
        if (measurementActive) {
            rangefinder.stopContinuousMeasurement();
            measurementActive = false;
            Serial.println("Measurement stopped");
            displayStatus("STOPPED", COLOR_STATUS_ERROR);
        } else {
            rangefinder.startContinuousMeasurement();
            measurementActive = true;
            Serial.println("Measurement started");
            displayStatus("RUNNING", COLOR_STATUS_OK);
        }
        delay(200);
    }

    // Read distance measurement
    if (measurementActive) {
        float distance;
        if (rangefinder.readContinuousDistance(distance)) {
            currentDistance = distance;
            measurementCount++;
            lastMeasurementTime = millis();

            // Print to serial for debugging
            if (measurementCount % 10 == 0) {
                Serial.print("Distance: ");
                Serial.print(currentDistance);
                Serial.print(" mm (");
                Serial.print(currentDistance / 10.0);
                Serial.print(" cm) - Count: ");
                Serial.println(measurementCount);
            }
        } else {
            // Check if we haven't received data for too long
            if (millis() - lastMeasurementTime > 2000) {
                errorCount++;
                if (errorCount % 10 == 0) {
                    Serial.println("Warning: No measurement data");
                }
            }
        }
    }

    // Update display at regular intervals
    if (millis() - lastUpdateTime >= DISPLAY_UPDATE_INTERVAL) {
        lastUpdateTime = millis();
        updateDistanceDisplay();

        // Update status indicator
        if (measurementActive) {
            displayStatus("RUNNING", COLOR_STATUS_OK);
        }
    }

    delay(10);
}

void displayStartupScreen() {
    M5.Display.fillScreen(COLOR_BG);
    M5.Display.setTextColor(TFT_CYAN);
    M5.Display.setFont(&fonts::FreeSansBold18pt7b);
    M5.Display.drawString("SEN0366", SCREEN_WIDTH / 2, 60);

    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setFont(&fonts::FreeSans12pt7b);
    M5.Display.drawString("Laser Rangefinder", SCREEN_WIDTH / 2, 100);

    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString("Initializing...", SCREEN_WIDTH / 2, 140);

    M5.Display.setTextColor(TFT_DARKGREY);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString("Port A: G32/G33", SCREEN_WIDTH / 2, 180);
}

void drawMainScreen() {
    M5.Display.fillScreen(COLOR_BG);

    // Draw title
    M5.Display.setTextColor(TFT_CYAN);
    M5.Display.setFont(&fonts::FreeSansBold12pt7b);
    M5.Display.drawString("DISTANCE", SCREEN_WIDTH / 2, 25);

    // Draw separator line
    M5.Display.drawLine(20, 45, SCREEN_WIDTH - 20, 45, TFT_DARKGREY);

    // Draw button labels at bottom
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString("Start/Stop", 40, 225);

    // Draw status label
    M5.Display.drawString("Status:", 20, 200);
}

void updateDistanceDisplay() {
    // Clear distance area
    M5.Display.fillRect(0, 50, SCREEN_WIDTH, 140, COLOR_BG);

    // Display distance in mm
    M5.Display.setTextColor(COLOR_DISTANCE);
    M5.Display.setFont(&fonts::FreeSansBold24pt7b);

    char distStr[20];
    if (currentDistance < 1000.0) {
        sprintf(distStr, "%.1f", currentDistance);
    } else if (currentDistance < 10000.0) {
        sprintf(distStr, "%.0f", currentDistance);
    } else {
        sprintf(distStr, "%.0f", currentDistance);
    }

    M5.Display.drawString(distStr, SCREEN_WIDTH / 2, 100);

    // Display unit
    M5.Display.setTextColor(COLOR_UNIT);
    M5.Display.setFont(&fonts::FreeSansBold12pt7b);
    M5.Display.drawString("mm", SCREEN_WIDTH / 2, 140);

    // Display in centimeters (smaller)
    M5.Display.setTextColor(TFT_DARKGREY);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    char cmStr[20];
    sprintf(cmStr, "(%.2f cm)", currentDistance / 10.0);
    M5.Display.drawString(cmStr, SCREEN_WIDTH / 2, 170);
}

void displayStatus(const char* status, uint16_t color) {
    // Clear status area
    M5.Display.fillRect(70, 190, 100, 20, COLOR_BG);

    // Draw status
    M5.Display.setTextColor(color);
    M5.Display.setFont(&fonts::FreeSansBold9pt7b);
    M5.Display.drawString(status, 120, 200);
}

void displayError(const char* errorMsg) {
    M5.Display.fillScreen(COLOR_BG);
    M5.Display.setTextColor(COLOR_STATUS_ERROR);
    M5.Display.setFont(&fonts::FreeSansBold12pt7b);
    M5.Display.drawString("ERROR", SCREEN_WIDTH / 2, 80);

    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setFont(&fonts::FreeSans9pt7b);
    M5.Display.drawString(errorMsg, SCREEN_WIDTH / 2, 120);
}
