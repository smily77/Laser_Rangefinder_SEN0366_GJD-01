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
 *   - TX (Rangefinder) -> G32 (RX on M5Stack)
 *   - RX (Rangefinder) -> G33 (TX on M5Stack)
 *   - VCC -> 5V
 *   - GND -> GND
 *
 * Features:
 * - Large, easy-to-read distance display
 * - Real-time measurement updates
 * - Beautiful font rendering
 * - Status indicators
 * - Visual laser indicator (red beam symbol)
 * - Audible beep for single measurements
 *
 * @author GJD-01
 * @date 2024
 */

#include <M5Unified.h>
#include <LaserRangefinder_SEN0366.h>

// Port A pins on M5Stack Core2
#define RXD2 32  // G32 (RX Pin)
#define TXD2 33  // G33 (TX Pin)

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
bool laserEnabled = false;
unsigned long lastUpdateTime = 0;
unsigned long lastMeasurementTime = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 100; // ms
int measurementCount = 0;
int errorCount = 0;

// Sprite for flicker-free distance display
LGFX_Sprite distanceSprite(&M5.Display);

void setup() {
    // Initialize M5Stack
    auto cfg = M5.config();
    M5.begin(cfg);

    // Initialize speaker for beep sounds
    M5.Speaker.begin();
    M5.Speaker.setVolume(128);  // Set medium volume (0-255)

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

    // IMPORTANT: For M5Stack Core2, use begin() with RX/TX pins for Port A
    // Port A: RX=G32 (RXD2), TX=G33 (TXD2)
    rangefinder.begin(9600, RXD2, TXD2);
    delay(500);

    // Enable debug mode to see communication details
    rangefinder.setDebug(true, &Serial);

    // Configure rangefinder for optimal continuous measurement
    Serial.println("Configuring rangefinder...");

    // Set resolution to 0.1mm for better accuracy
    if (rangefinder.setResolution(SEN0366_RESOLUTION_01MM)) {
        Serial.println("Resolution set to 0.1mm");
    } else {
        Serial.println("Warning: Could not set resolution");
    }

    delay(200);

    // Set measurement frequency to 10Hz
    if (rangefinder.setFrequency(SEN0366_FREQ_10HZ)) {
        Serial.println("Frequency set to 10Hz");
    } else {
        Serial.println("Warning: Could not set frequency");
    }

    delay(200);

    // Initialize sprite for flicker-free distance display
    distanceSprite.createSprite(SCREEN_WIDTH, 140);
    distanceSprite.setTextDatum(middle_center);

    // Draw main screen in Idle state
    drawMainScreen();
    displayStatus("IDLE", TFT_DARKGREY);
    Serial.println("Ready - Press buttons to start");
}

void loop() {
    M5.update();

    // Left button (A): Laser On/Off
    if (M5.BtnA.wasPressed()) {
        if (laserEnabled) {
            rangefinder.controlLaser(SEN0366_LASER_OFF);
            laserEnabled = false;
            Serial.println("Laser turned OFF");
        } else {
            rangefinder.controlLaser(SEN0366_LASER_ON);
            laserEnabled = true;
            Serial.println("Laser turned ON");
        }
        updateLaserSymbol();  // Update laser symbol display
        delay(200);
    }

    // Middle button (B): Single measurement (only when continuous mode is stopped)
    if (M5.BtnB.wasPressed() && !measurementActive) {
        Serial.println("Taking single measurement...");
        displayStatus("MEASURING", TFT_YELLOW);

        // Laser turns on automatically during single measurement
        laserEnabled = true;
        updateLaserSymbol();  // Show laser symbol

        float distance;
        if (rangefinder.singleMeasurement(distance)) {
            currentDistance = distance;
            lastMeasurementTime = millis();
            Serial.print("Single measurement: ");
            Serial.print(currentDistance, 3);
            Serial.println(" m");
            updateDistanceDisplay();

            // Play a short beep to indicate successful measurement
            M5.Speaker.tone(2000, 100);  // 2kHz tone for 100ms

            // Laser turns off automatically after single measurement
            laserEnabled = false;
            updateLaserSymbol();  // Hide laser symbol
            displayStatus("IDLE", TFT_DARKGREY);
        } else {
            Serial.println("Single measurement failed");
            // Laser turns off after failed measurement
            laserEnabled = false;
            updateLaserSymbol();  // Hide laser symbol
            displayStatus("ERROR", COLOR_STATUS_ERROR);
        }
        delay(200);
    }

    // Right button (C): Start/Stop continuous measurement
    if (M5.BtnC.wasPressed()) {
        if (measurementActive) {
            rangefinder.stopContinuousMeasurement();
            measurementActive = false;
            // Turn off laser when stopping continuous measurement
            if (laserEnabled) {
                rangefinder.controlLaser(SEN0366_LASER_OFF);
                laserEnabled = false;
                updateLaserSymbol();  // Hide laser symbol
            }
            Serial.println("Continuous measurement stopped, laser OFF");
            displayStatus("IDLE", TFT_DARKGREY);
        } else {
            rangefinder.startContinuousMeasurement();
            measurementActive = true;
            measurementCount = 0; // Reset count
            // Laser turns on automatically during continuous measurement
            laserEnabled = true;
            updateLaserSymbol();  // Show laser symbol
            Serial.println("Continuous measurement started, laser ON");
            displayStatus("RUNNING", COLOR_STATUS_OK);
        }
        delay(200);
    }

    // Read distance measurement in continuous mode
    if (measurementActive) {
        float distance;
        if (rangefinder.readContinuousDistance(distance)) {
            currentDistance = distance;
            measurementCount++;
            lastMeasurementTime = millis();
            errorCount = 0; // Reset error count on successful read

            // Print to serial for debugging
            // IMPORTANT: Sensor returns distance in METERS
            if (measurementCount % 10 == 0) {
                Serial.print("Distance: ");
                Serial.print(currentDistance, 3);
                Serial.print(" m (");
                Serial.print(currentDistance * 100.0, 1);
                Serial.print(" cm / ");
                Serial.print(currentDistance * 1000.0, 0);
                Serial.print(" mm) - Count: ");
                Serial.println(measurementCount);
            }
        } else {
            // Check if we haven't received data for too long
            if (measurementCount > 0 && millis() - lastMeasurementTime > 2000) {
                errorCount++;
                if (errorCount % 20 == 0) {
                    Serial.println("Warning: No measurement data received");
                    Serial.print("Error count: ");
                    Serial.println(errorCount);
                }
            }
        }
    }

    // Update display at regular intervals
    if (millis() - lastUpdateTime >= DISPLAY_UPDATE_INTERVAL) {
        lastUpdateTime = millis();
        updateDistanceDisplay();

        // Update status indicator based on current state
        if (measurementActive) {
            displayStatus("RUNNING", COLOR_STATUS_OK);
        }
    }

    delay(10);
}

void drawLaserSymbol(int x, int y) {
    // Draw a small laser beam symbol (red circle with rays)
    M5.Display.fillCircle(x, y, 5, TFT_RED);
    M5.Display.drawLine(x - 10, y, x - 6, y, TFT_RED);
    M5.Display.drawLine(x + 6, y, x + 10, y, TFT_RED);
    M5.Display.drawLine(x - 7, y - 7, x - 4, y - 4, TFT_RED);
    M5.Display.drawLine(x + 4, y + 4, x + 7, y + 7, TFT_RED);
}

void updateLaserSymbol() {
    // Clear the laser symbol area (moved 15 pixels to the right)
    M5.Display.fillRect(SCREEN_WIDTH / 2 + 80, 15, 25, 20, COLOR_BG);

    // Draw laser symbol if enabled (moved 15 pixels to the right)
    if (laserEnabled) {
        drawLaserSymbol(SCREEN_WIDTH / 2 + 90, 25);
    }
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

    // Draw laser symbol if enabled
    updateLaserSymbol();

    // Draw separator line
    M5.Display.drawLine(20, 45, SCREEN_WIDTH - 20, 45, TFT_DARKGREY);

    // Draw button labels at bottom
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString("Laser", 40, 225);
    M5.Display.drawString("Single", 160, 225);
    M5.Display.drawString("Start/Stop", 280, 225);

    // Draw status label
    M5.Display.drawString("Status:", 20, 200);
}

void updateDistanceDisplay() {
    // Clear sprite
    distanceSprite.fillSprite(COLOR_BG);

    // Display distance in meters (sensor returns METERS!)
    distanceSprite.setTextColor(COLOR_DISTANCE);
    distanceSprite.setFont(&fonts::FreeSansBold24pt7b);

    char distStr[20];
    // Format distance in meters with appropriate precision
    if (currentDistance < 1.0) {
        sprintf(distStr, "%.3f", currentDistance);
    } else if (currentDistance < 10.0) {
        sprintf(distStr, "%.2f", currentDistance);
    } else {
        sprintf(distStr, "%.1f", currentDistance);
    }

    distanceSprite.drawString(distStr, SCREEN_WIDTH / 2, 50);

    // Display unit
    distanceSprite.setTextColor(COLOR_UNIT);
    distanceSprite.setFont(&fonts::FreeSansBold12pt7b);
    distanceSprite.drawString("m", SCREEN_WIDTH / 2, 90);

    // Display in centimeters (smaller)
    distanceSprite.setTextColor(TFT_DARKGREY);
    distanceSprite.setFont(&fonts::FreeSans9pt7b);
    char cmStr[32];  // Increased buffer size to prevent overflow
    sprintf(cmStr, "(%.1f cm / %.0f mm)", currentDistance * 100.0, currentDistance * 1000.0);
    distanceSprite.drawString(cmStr, SCREEN_WIDTH / 2, 120);

    // Push sprite to display in one operation (prevents flickering)
    distanceSprite.pushSprite(0, 50);
}

void displayStatus(const char* status, uint16_t color) {
    // Clear status area (wider to prevent artifacts from longer words like "MEASURING")
    M5.Display.fillRect(70, 190, 160, 20, COLOR_BG);

    // Draw status
    M5.Display.setTextColor(color);
    M5.Display.setFont(&fonts::FreeSansBold9pt7b);
    M5.Display.drawString(status, 150, 200);
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
