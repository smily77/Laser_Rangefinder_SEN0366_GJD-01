/**
 * @file AdvancedSettings.ino
 * @brief Advanced configuration and multi-device example for SEN0366
 *
 * This example demonstrates:
 * - Setting device address for multi-device setups
 * - Configuring all rangefinder parameters
 * - Reading device information
 * - Using synchronized measurements with cache
 * - Controlling laser on/off
 *
 * Hardware Setup:
 * - Connect TX (Rangefinder) to RX pin (default: pin 16)
 * - Connect RX (Rangefinder) to TX pin (default: pin 17)
 * - Connect VCC to 5V
 * - Connect GND to GND
 *
 * For multi-device setup:
 * - Connect all rangefinders in parallel to the same serial pins
 * - Use setAddress() to assign unique addresses to each device
 *   (connect only one at a time when setting addresses)
 *
 * @author GJD-01
 * @date 2024
 */

#include <LaserRangefinder_SEN0366.h>

#if defined(ESP32)
  #define RXD2 16
  #define TXD2 17
  LaserRangefinder_SEN0366 rangefinder(&Serial2);
#elif defined(__AVR_ATmega2560__)
  LaserRangefinder_SEN0366 rangefinder(&Serial1);
#else
  #error "Please define serial pins for your board"
#endif

// Menu state
int menuSelection = 0;
bool menuActive = true;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("\n\n");
  Serial.println("=============================================");
  Serial.println("  SEN0366 Advanced Configuration Demo");
  Serial.println("=============================================");
  Serial.println();

  // Initialize rangefinder
  #if defined(ESP32)
    // For ESP32: Use begin() with RX/TX pins
    rangefinder.begin(9600, RXD2, TXD2);
  #elif defined(__AVR_ATmega2560__)
    // For Arduino Mega: Serial1 has fixed pins
    rangefinder.begin(9600);
  #else
    rangefinder.begin(9600);
  #endif

  delay(500);

  // Enable debug output
  rangefinder.setDebug(true);

  displayMenu();
}

void loop() {
  if (menuActive && Serial.available()) {
    char choice = Serial.read();

    switch (choice) {
      case '1':
        readDeviceInfo();
        break;
      case '2':
        configureResolution();
        break;
      case '3':
        configureFrequency();
        break;
      case '4':
        configureMeasurementPoint();
        break;
      case '5':
        configureAutoStart();
        break;
      case '6':
        setDeviceAddress();
        break;
      case '7':
        demonstrateSingleMeasurement();
        break;
      case '8':
        demonstrateContinuousMeasurement();
        break;
      case '9':
        demonstrateSynchronizedMeasurement();
        break;
      case 'a':
      case 'A':
        testLaserControl();
        break;
      case 'm':
      case 'M':
        displayMenu();
        break;
      case 'q':
      case 'Q':
        Serial.println("\nExiting menu mode...");
        menuActive = false;
        break;
    }

    // Clear input buffer
    while (Serial.available()) Serial.read();
  }

  delay(100);
}

void displayMenu() {
  Serial.println("\n╔═══════════════════════════════════════════╗");
  Serial.println("║          MAIN MENU                        ║");
  Serial.println("╠═══════════════════════════════════════════╣");
  Serial.println("║ Device Information:                       ║");
  Serial.println("║   [1] Read device info & serial number    ║");
  Serial.println("║                                           ║");
  Serial.println("║ Configuration:                            ║");
  Serial.println("║   [2] Set resolution (1mm / 0.1mm)        ║");
  Serial.println("║   [3] Set frequency (0/5/10/20 Hz)        ║");
  Serial.println("║   [4] Set measurement starting point      ║");
  Serial.println("║   [5] Set auto-start on power-up          ║");
  Serial.println("║   [6] Set device address (multi-device)   ║");
  Serial.println("║                                           ║");
  Serial.println("║ Measurements:                             ║");
  Serial.println("║   [7] Single measurement demo             ║");
  Serial.println("║   [8] Continuous measurement demo         ║");
  Serial.println("║   [9] Synchronized measurement (cache)    ║");
  Serial.println("║                                           ║");
  Serial.println("║ Control:                                  ║");
  Serial.println("║   [A] Test laser on/off control           ║");
  Serial.println("║                                           ║");
  Serial.println("║   [M] Show this menu                      ║");
  Serial.println("║   [Q] Quit menu mode                      ║");
  Serial.println("╚═══════════════════════════════════════════╝");
  Serial.println("\nEnter your choice: ");
}

void readDeviceInfo() {
  Serial.println("\n--- Reading Device Information ---");

  char serialNumber[17];
  if (rangefinder.readMachineNumber(serialNumber)) {
    Serial.print("✓ Device Serial Number: ");
    Serial.println(serialNumber);
  } else {
    Serial.println("✗ Failed to read serial number");
  }

  Serial.println("\nCurrent device address: 0x");
  Serial.println(rangefinder.getDeviceAddress(), HEX);
  Serial.print("Using broadcast: ");
  Serial.println(rangefinder.isBroadcast() ? "Yes" : "No");

  Serial.println("\nPress any key to continue...");
  waitForKey();
}

void configureResolution() {
  Serial.println("\n--- Configure Resolution ---");
  Serial.println("1 = 1mm resolution (faster)");
  Serial.println("2 = 0.1mm resolution (more precise)");
  Serial.print("Enter choice: ");

  char choice = waitForKey();
  Serial.println(choice);

  uint8_t resolution = (choice == '1') ? SEN0366_RESOLUTION_1MM : SEN0366_RESOLUTION_01MM;

  if (rangefinder.setResolution(resolution)) {
    Serial.println("✓ Resolution configured successfully");
  } else {
    Serial.println("✗ Failed to configure resolution");
  }

  delay(1000);
}

void configureFrequency() {
  Serial.println("\n--- Configure Measurement Frequency ---");
  Serial.println("0 = 0 Hz");
  Serial.println("1 = 5 Hz");
  Serial.println("2 = 10 Hz");
  Serial.println("3 = 20 Hz");
  Serial.print("Enter choice: ");

  char choice = waitForKey();
  Serial.println(choice);

  uint8_t frequency;
  switch (choice) {
    case '0': frequency = SEN0366_FREQ_0HZ; break;
    case '1': frequency = SEN0366_FREQ_5HZ; break;
    case '2': frequency = SEN0366_FREQ_10HZ; break;
    case '3': frequency = SEN0366_FREQ_20HZ; break;
    default:
      Serial.println("Invalid choice");
      return;
  }

  if (rangefinder.setFrequency(frequency)) {
    Serial.println("✓ Frequency configured successfully");
  } else {
    Serial.println("✗ Failed to configure frequency");
  }

  delay(1000);
}

void configureMeasurementPoint() {
  Serial.println("\n--- Configure Measurement Starting Point ---");
  Serial.println("1 = Measure from front (laser exit)");
  Serial.println("2 = Measure from rear");
  Serial.print("Enter choice: ");

  char choice = waitForKey();
  Serial.println(choice);

  uint8_t position = (choice == '1') ? SEN0366_MEASURE_FROM_FRONT : SEN0366_MEASURE_FROM_REAR;

  if (rangefinder.setMeasurementStartingPoint(position)) {
    Serial.println("✓ Measurement point configured successfully");
  } else {
    Serial.println("✗ Failed to configure measurement point");
  }

  delay(1000);
}

void configureAutoStart() {
  Serial.println("\n--- Configure Auto-Start on Power-Up ---");
  Serial.println("1 = Enable auto-start");
  Serial.println("2 = Disable auto-start");
  Serial.print("Enter choice: ");

  char choice = waitForKey();
  Serial.println(choice);

  uint8_t autoStart = (choice == '1') ? SEN0366_AUTOSTART_ENABLE : SEN0366_AUTOSTART_DISABLE;

  if (rangefinder.setAutoStart(autoStart)) {
    Serial.println("✓ Auto-start configured successfully");
  } else {
    Serial.println("✗ Failed to configure auto-start");
  }

  delay(1000);
}

void setDeviceAddress() {
  Serial.println("\n--- Set Device Address (Multi-Device Setup) ---");
  Serial.println("WARNING: Only connect ONE rangefinder when setting address!");
  Serial.println("\nValid addresses: 0x80 to 0xF9");
  Serial.println("Example: Enter '80' for address 0x80");
  Serial.print("Enter hex address (2 digits): ");

  String input = "";
  while (input.length() < 2) {
    if (Serial.available()) {
      char c = Serial.read();
      Serial.print(c);
      input += c;
    }
  }
  Serial.println();

  // Convert hex string to number
  uint8_t address = strtol(input.c_str(), NULL, 16);

  if (address < SEN0366_MIN_ADDR || address > SEN0366_MAX_ADDR) {
    Serial.println("✗ Invalid address");
    return;
  }

  Serial.print("Setting address to 0x");
  Serial.println(address, HEX);

  if (rangefinder.setAddress(address)) {
    Serial.println("✓ Address set successfully");
    Serial.println("The rangefinder now has individual address 0x");
    Serial.println(address, HEX);
  } else {
    Serial.println("✗ Failed to set address");
  }

  delay(2000);
}

void demonstrateSingleMeasurement() {
  Serial.println("\n--- Single Measurement Demo ---");
  Serial.println("Taking 5 measurements...\n");

  for (int i = 1; i <= 5; i++) {
    Serial.print("Measurement ");
    Serial.print(i);
    Serial.print(": ");

    float distance;
    // IMPORTANT: Sensor returns distance in METERS!
    if (rangefinder.singleMeasurement(distance)) {
      Serial.print(distance, 3);
      Serial.print(" m (");
      Serial.print(distance * 100.0, 1);
      Serial.print(" cm / ");
      Serial.print(distance * 1000.0, 0);
      Serial.println(" mm)");
    } else {
      Serial.println("FAILED");
    }

    delay(500);
  }

  Serial.println("\nPress any key to continue...");
  waitForKey();
}

void demonstrateContinuousMeasurement() {
  Serial.println("\n--- Continuous Measurement Demo ---");
  Serial.println("Starting continuous measurement for 10 seconds...");
  Serial.println("Press any key to stop early\n");

  if (!rangefinder.startContinuousMeasurement()) {
    Serial.println("✗ Failed to start continuous measurement");
    return;
  }

  unsigned long startTime = millis();
  int count = 0;

  while (millis() - startTime < 10000 && !Serial.available()) {
    float distance;
    // IMPORTANT: Sensor returns distance in METERS!
    if (rangefinder.readContinuousDistance(distance)) {
      count++;
      Serial.print("[");
      Serial.print(count);
      Serial.print("] ");
      Serial.print(distance, 3);
      Serial.print(" m (");
      Serial.print(distance * 100.0, 1);
      Serial.print(" cm / ");
      Serial.print(distance * 1000.0, 0);
      Serial.println(" mm)");
    }
    delay(50);
  }

  rangefinder.stopContinuousMeasurement();
  Serial.println("\n✓ Continuous measurement stopped");
  Serial.print("Total measurements: ");
  Serial.println(count);

  // Clear input buffer
  while (Serial.available()) Serial.read();

  Serial.println("\nPress any key to continue...");
  waitForKey();
}

void demonstrateSynchronizedMeasurement() {
  Serial.println("\n--- Synchronized Measurement Demo (Cache) ---");
  Serial.println("This feature is for multi-device synchronized readings");
  Serial.println("\n1. Broadcast command to take measurement");
  Serial.println("2. Read cached value from device");
  Serial.println();

  // Trigger measurement on all devices (broadcast)
  Serial.println("Triggering measurement on all devices...");
  rangefinder.takeSingleMeasurementToCache();

  delay(500); // Wait for measurement to complete

  // Read cached value
  Serial.println("Reading cached value...");
  float distance;
  // IMPORTANT: Sensor returns distance in METERS!
  if (rangefinder.readCache(distance)) {
    Serial.print("✓ Cached distance: ");
    Serial.print(distance, 3);
    Serial.print(" m (");
    Serial.print(distance * 100.0, 1);
    Serial.print(" cm / ");
    Serial.print(distance * 1000.0, 0);
    Serial.println(" mm)");
  } else {
    Serial.println("✗ Failed to read cache");
  }

  Serial.println("\nPress any key to continue...");
  waitForKey();
}

void testLaserControl() {
  Serial.println("\n--- Laser Control Test ---");

  Serial.println("Turning laser OFF...");
  if (rangefinder.controlLaser(SEN0366_LASER_OFF)) {
    Serial.println("✓ Laser turned OFF");
  } else {
    Serial.println("✗ Failed to control laser");
  }

  delay(2000);

  Serial.println("Turning laser ON...");
  if (rangefinder.controlLaser(SEN0366_LASER_ON)) {
    Serial.println("✓ Laser turned ON");
  } else {
    Serial.println("✗ Failed to control laser");
  }

  Serial.println("\nPress any key to continue...");
  waitForKey();
}

char waitForKey() {
  while (!Serial.available()) {
    delay(10);
  }
  char c = Serial.read();
  // Clear remaining buffer
  delay(100);
  while (Serial.available()) Serial.read();
  return c;
}
