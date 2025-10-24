/**
 * @file BasicExample.ino
 * @brief Minimales Beispiel für SEN0366 - ähnlich wie Wiki
 *
 * Dies ist das einfachste mögliche Beispiel.
 * Verwenden Sie dies als Startpunkt für Ihre eigenen Projekte.
 *
 * Hardware Setup für M5Stack Core2:
 * - SEN0366 TX -> G33 (RX)
 * - SEN0366 RX -> G32 (TX)
 * - SEN0366 VCC -> 5V
 * - SEN0366 GND -> GND
 *
 * Hardware Setup für andere ESP32:
 * - SEN0366 TX -> GPIO 16 (RX)
 * - SEN0366 RX -> GPIO 17 (TX)
 * - SEN0366 VCC -> 5V
 * - SEN0366 GND -> GND
 */

#include <LaserRangefinder_SEN0366.h>

// Definiere deine RX und TX Pins hier
// Für M5Stack Core2 Port A:
#define RXD 32  // G32 (RX Pin)
#define TXD 33  // G33 (TX Pin)

// Für andere ESP32 Boards, verwende z.B.:
// #define RXD 16
// #define TXD 17

// Erstelle Rangefinder Objekt
LaserRangefinder_SEN0366 rangefinder(&Serial2);

void setup() {
  // USB Serial für Ausgabe
  Serial.begin(115200);
  delay(1000);

  Serial.println("SEN0366 Basic Example");
  Serial.println("Initialisiere Sensor...");

  // WICHTIG: Für ESP32 MUSS man RXD und TXD Pins übergeben!
  rangefinder.begin(9600, RXD, TXD);

  // Optional: Debug-Modus aktivieren um Rohdaten zu sehen
  // rangefinder.setDebug(true);

  Serial.println("Sensor bereit!");
  Serial.println();
}

void loop() {
  float distance;

  // Einzelmessung durchführen
  // WICHTIG: Der Sensor liefert die Distanz in METERN!
  if (rangefinder.singleMeasurement(distance)) {
    // Erfolgreich gemessen
    Serial.print("Distanz: ");
    Serial.print(distance, 3);
    Serial.print(" m = ");
    Serial.print(distance * 100.0, 1);
    Serial.print(" cm = ");
    Serial.print(distance * 1000.0, 0);
    Serial.println(" mm");
  } else {
    // Fehler beim Messen
    Serial.println("Messfehler!");
  }

  // Warte 1 Sekunde vor nächster Messung
  delay(1000);
}
