/**
 * @file SingleMeasurement.ino
 * @brief Basic single measurement example for SEN0366 Laser Rangefinder
 *
 * This example demonstrates how to perform single distance measurements
 * using the SEN0366 Laser Rangefinder.
 *
 * Hardware Setup:
 * - Connect TX (Rangefinder) to RX pin (default: pin 16)
 * - Connect RX (Rangefinder) to TX pin (default: pin 17)
 * - Connect VCC to 5V
 * - Connect GND to GND
 *
 * This example works with any Arduino board with hardware serial support.
 *
 * @author GJD-01
 * @date 2024
 */

#include <LaserRangefinder_SEN0366.h>

// Define RX and TX pins for your board
// For ESP32: Use Serial2 with pins 16 (RX) and 17 (TX)
// For Arduino Mega: Use Serial1
// For Arduino Uno: Use SoftwareSerial

#if defined(ESP32)
  #define RXD2 32
  #define TXD2 33
  LaserRangefinder_SEN0366 rangefinder(&Serial2);
#elif defined(__AVR_ATmega2560__)
  LaserRangefinder_SEN0366 rangefinder(&Serial1);
#else
  #error "Please define serial pins for your board"
#endif

void setup() {
  // Initialize USB serial for debug output
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("=================================");
  Serial.println("SEN0366 Single Measurement Demo");
  Serial.println("=================================");
  Serial.println();

  // Initialize rangefinder
  Serial.println("Initializing rangefinder...");

  #if defined(ESP32)
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  #endif

  rangefinder.begin(9600);
  delay(500);

  // Optional: Enable debug output
  // rangefinder.setDebug(true);

  // Configure rangefinder settings
  Serial.println("Configuring rangefinder...");
/*
  // Set resolution to 1mm (faster) or 0.1mm (more precise)
  if (rangefinder.setResolution(SEN0366_RESOLUTION_1MM)) {
    Serial.println("✓ Resolution set to 1mm");
  } else {
    Serial.println("✗ Failed to set resolution");
  }

  delay(100);
*/
  Serial.println();
  Serial.println("Ready! Taking measurements every 2 seconds...");
  Serial.println("Distance readings:");
  Serial.println("----------------------------------");
}

void loop() {
  float distance;

  // Perform single measurement
  if (rangefinder.singleMeasurement(distance)) {
    // Display result
    Serial.print("Distance: ");
    Serial.print(distance, 1);
    Serial.print(" mm  |  ");
    Serial.print(distance / 10.0, 2);
    Serial.print(" cm  |  ");
    Serial.print(distance / 1000.0, 3);
    Serial.println(" m");

    // Optional: Categorize distance
    if (distance < 100.0) {
      Serial.println("  → Very close!");
    } else if (distance < 1000.0) {
      Serial.println("  → Close range");
    } else if (distance < 10000.0) {
      Serial.println("  → Medium range");
    } else {
      Serial.println("  → Long range");
    }
  } else {
    Serial.println("✗ Measurement failed - no response or error");
  }

  Serial.println();

  // Wait before next measurement
  delay(2000);
}
