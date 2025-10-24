# LaserRangefinder_SEN0366

Arduino-Bibliothek für den DFRobot SEN0366 Laser-Entfernungsmesser

## Übersicht

Diese vollständige Arduino-Bibliothek implementiert das komplette UART-Protokoll für den SEN0366 Infrarot-Laser-Entfernungsmesser (50m/80m). Die Bibliothek unterstützt sowohl Einzelmessungen als auch kontinuierliche Messungen und bietet Unterstützung für Broadcast- und individuelle Adressierung bei Mehrgeräte-Setups.

## Features

- ✅ Vollständige Implementierung des SEN0366 UART-Protokolls
- ✅ Single und Continuous Measurement Modi
- ✅ Broadcast-Adressierung (0xFA) für Einzelgerät-Systeme
- ✅ Individuelle Adressierung (0x80-0xF9) für Mehrgerät-Systeme
- ✅ Alle Konfigurationskommandos implementiert
- ✅ Synchronisierte Messungen mit Cache-Funktion
- ✅ Auflösungseinstellungen (1mm / 0.1mm)
- ✅ Frequenzeinstellungen (0, 5, 10, 20 Hz)
- ✅ Laser Ein/Aus Kontrolle
- ✅ Debug-Modus für Entwicklung
- ✅ Kompatibel mit ESP32, Arduino Mega, und anderen Boards
- ✅ Spezielle Unterstützung für M5Stack Core2

## Hardware-Anforderungen

### Sensor
- DFRobot SEN0366 Infrarot-Laser-Entfernungsmesser

### Verbindung
- UART/Serial Schnittstelle (9600 Baud, 8N1)
- 3.3V oder 5V TTL Level
- 4 Leitungen: VCC, GND, TX, RX

### Beispiel-Verbindung für M5Stack Core2 (Port A)
```
SEN0366    →  M5Stack Core2
TX         →  G32 (RX)
RX         →  G33 (TX)
VCC        →  5V
GND        →  GND
```

### Beispiel-Verbindung für ESP32
```
SEN0366    →  ESP32
TX         →  GPIO 16 (RX)
RX         →  GPIO 17 (TX)
VCC        →  5V
GND        →  GND
```

## Installation

### Arduino IDE

1. **Download**: Klonen Sie das Repository oder laden Sie es als ZIP herunter
   ```bash
   git clone https://github.com/smily77/Laser_Rangefinder_SEN0366_GJD-01.git
   ```

2. **Bibliothek installieren**:
   - Öffnen Sie die Arduino IDE
   - Gehen Sie zu `Sketch` → `Include Library` → `Add .ZIP Library...`
   - Wählen Sie die heruntergeladene ZIP-Datei oder den Ordner aus

3. **Beispiele**: Nach der Installation finden Sie die Beispiele unter `File` → `Examples` → `LaserRangefinder_SEN0366`

## Verwendung

### Einfaches Beispiel - Einzelmessung

**WICHTIG für ESP32/M5Stack:** Die RX/TX Pins müssen bei `begin()` übergeben werden!

**WICHTIG:** Der Sensor liefert die Distanz in **METERN**, nicht in mm!

```cpp
#include <LaserRangefinder_SEN0366.h>

// Für M5Stack Core2 Port A:
#define RXD 32  // G32 (RX Pin)
#define TXD 33  // G33 (TX Pin)

// Für andere ESP32:
// #define RXD 16
// #define TXD 17

LaserRangefinder_SEN0366 rangefinder(&Serial2);

void setup() {
  Serial.begin(115200);

  // RICHTIG: Pins bei begin() übergeben (ESP32)
  rangefinder.begin(9600, RXD, TXD);

  rangefinder.setResolution(SEN0366_RESOLUTION_1MM);
}

void loop() {
  float distance;

  // WICHTIG: distance ist in METERN!
  if (rangefinder.singleMeasurement(distance)) {
    Serial.print("Entfernung: ");
    Serial.print(distance, 3);
    Serial.print(" m (");
    Serial.print(distance * 1000.0, 0);
    Serial.println(" mm)");
  }

  delay(1000);
}
```

### Kontinuierliche Messung

```cpp
#include <LaserRangefinder_SEN0366.h>

#define RXD 32  // Für M5Stack Core2 (RX Pin)
#define TXD 33  // (TX Pin)

LaserRangefinder_SEN0366 rangefinder(&Serial2);

void setup() {
  Serial.begin(115200);

  // Pins bei begin() übergeben
  rangefinder.begin(9600, RXD, TXD);

  rangefinder.setFrequency(SEN0366_FREQ_10HZ);
  rangefinder.startContinuousMeasurement();
}

void loop() {
  float distance;

  // WICHTIG: distance ist in METERN!
  if (rangefinder.readContinuousDistance(distance)) {
    Serial.print("Entfernung: ");
    Serial.print(distance, 3);
    Serial.print(" m (");
    Serial.print(distance * 1000.0, 0);
    Serial.println(" mm)");
  }

  delay(10);
}
```

### Mehrgerät-Setup

```cpp
// Adresse setzen (nur ein Gerät anschließen!)
rangefinder.setAddress(0x80);

// Danach mit spezifischer Adresse arbeiten
LaserRangefinder_SEN0366 rangefinder1(&Serial2, 0x80);
LaserRangefinder_SEN0366 rangefinder2(&Serial2, 0x81);

// Synchronisierte Messung
rangefinder1.takeSingleMeasurementToCache(); // Broadcast an alle

float dist1, dist2;
rangefinder1.readCache(dist1);
rangefinder2.readCache(dist2);
```

## API-Referenz

### Konstruktor

```cpp
LaserRangefinder_SEN0366(HardwareSerial* serial);
LaserRangefinder_SEN0366(HardwareSerial* serial, uint8_t address);
```

### Initialisierung

```cpp
void begin(uint32_t baudRate = 9600);
```

### Messfunktionen

```cpp
bool singleMeasurement(float& distance);
bool startContinuousMeasurement();
bool readContinuousDistance(float& distance);
bool stopContinuousMeasurement();
void takeSingleMeasurementToCache();
bool readCache(float& distance);
```

### Konfiguration

```cpp
bool setAddress(uint8_t newAddress);
bool setResolution(uint8_t resolution);
bool setFrequency(uint8_t frequency);
bool setDataReturnInterval(uint8_t interval);
bool setMeasurementStartingPoint(uint8_t position);
bool setAutoStart(uint8_t enable);
```

### Kontrolle

```cpp
bool controlLaser(uint8_t on);
bool shutDown();
```

### Information

```cpp
bool readMachineNumber(char* serialNumber);
uint8_t readParameter(uint8_t* data, uint8_t maxLen);
```

### Konstanten

```cpp
// Adressen
SEN0366_BROADCAST_ADDR   // 0xFA
SEN0366_MIN_ADDR         // 0x80
SEN0366_MAX_ADDR         // 0xF9

// Auflösung
SEN0366_RESOLUTION_1MM   // 1mm
SEN0366_RESOLUTION_01MM  // 0.1mm

// Frequenz
SEN0366_FREQ_0HZ         // 0 Hz
SEN0366_FREQ_5HZ         // 5 Hz
SEN0366_FREQ_10HZ        // 10 Hz
SEN0366_FREQ_20HZ        // 20 Hz

// Messpunkt
SEN0366_MEASURE_FROM_FRONT
SEN0366_MEASURE_FROM_REAR

// Auto-Start
SEN0366_AUTOSTART_ENABLE
SEN0366_AUTOSTART_DISABLE

// Laser
SEN0366_LASER_ON
SEN0366_LASER_OFF
```

## Beispiele

Die Bibliothek enthält drei vollständige Beispiele:

### 1. M5Stack_ContinuousMeasurement
Zeigt kontinuierliche Messungen auf einem M5Stack Core2 mit schöner grafischer Anzeige. Verwendet M5Unified.h und hochwertige Fonts.

**Features:**
- Große, gut lesbare Entfernungsanzeige
- Echtzeit-Updates
- Schöne Font-Darstellung
- Status-Indikatoren
- Start/Stop-Funktion

### 2. SingleMeasurement
Grundlegendes Beispiel für Einzelmessungen. Funktioniert mit jedem Arduino-Board mit Hardware-Serial-Unterstützung.

**Features:**
- Einfache Einzelmessungen
- Ausgabe über Serial Monitor
- Anzeige in mm, cm und m
- Distanz-Kategorisierung

### 3. AdvancedSettings
Umfassendes Beispiel mit interaktivem Menü für alle Funktionen.

**Features:**
- Interaktives Menü
- Geräteinformationen auslesen
- Alle Konfigurationsoptionen
- Mehrgerät-Setup
- Laser-Kontrolle
- Synchronisierte Messungen

## Protokoll-Details

### UART-Parameter
- Baudrate: 9600
- Data Bits: 8
- Parity: None
- Stop Bits: 1
- Voltage Level: 3.3V TTL

### Nachrichtenformat
```
<ADDR> <GRP> <CMD> [<DATA>] <CS>
```

- `ADDR`: Geräteadresse (0x80-0xF9) oder Broadcast (0xFA)
- `GRP`: Kommando-Gruppe
- `CMD`: Kommando-Funktion
- `DATA`: Optional, abhängig vom Kommando
- `CS`: Checksum (~SUM + 1)

### Messbereich
- Innenbereich: 0.05 - 80m
- Außenbereich: 0.05 - 50m

### Genauigkeit
- ±1% (bei guten Bedingungen)
- Abhängig von Oberflächenreflexion und Umgebungslicht

## Fehlerbehebung

### ⚠️ WICHTIG: Sensor antwortet nicht auf ESP32/M5Stack
**Häufigstes Problem:** Die RX/TX Pins wurden nicht korrekt initialisiert!

**FALSCH** ❌:
```cpp
Serial2.begin(9600, SERIAL_8N1, RXD, TXD);
rangefinder.begin(9600);  // Überschreibt die Pins!
```

**RICHTIG** ✅:
```cpp
rangefinder.begin(9600, RXD, TXD);  // Pins direkt übergeben
```

**Alternative** ✅:
```cpp
Serial2.begin(9600, SERIAL_8N1, RXD, TXD);
rangefinder.begin();  // Ohne Parameter - nutzt existierende Initialisierung
```

### Keine Antwort vom Sensor
1. **Für ESP32:** Verwenden Sie `rangefinder.begin(9600, RXD, TXD)` mit den richtigen Pins!
2. Überprüfen Sie die Verkabelung:
   - SEN0366 TX → ESP32 RX
   - SEN0366 RX → ESP32 TX
   - **Nicht** direkt kreuzen!
3. Stellen Sie sicher, dass 9600 Baud verwendet wird
4. Prüfen Sie die Stromversorgung (5V, min. 200mA)
5. Aktivieren Sie den Debug-Modus: `rangefinder.setDebug(true)`

### Falsche Messwerte
1. Überprüfen Sie die Auflösungseinstellung
2. Stellen Sie sicher, dass das Zielobjekt reflektierend ist
3. Vermeiden Sie direkte Sonneneinstrahlung
4. Erhöhen Sie das Timeout: `rangefinder.setTimeout(2000)`

### M5Stack Core2 spezifische Probleme
1. Port A nutzt: G32 (RX) und G33 (TX)
2. Verwenden Sie: `rangefinder.begin(9600, 32, 33);` (RX=32, TX=33)
3. Installieren Sie M5Unified.h: `Sketch` → `Include Library` → `Manage Libraries` → Suche "M5Unified"
4. Stellen Sie sicher, dass Serial2 verfügbar ist

### WICHTIG: Einheit der Distanz
**Der Sensor liefert die Distanz in METERN, nicht in Millimetern!**
- `distance` Wert = Meter
- Um mm zu erhalten: `distance * 1000.0`
- Um cm zu erhalten: `distance * 100.0`

## Lizenz

MIT License - Siehe LICENSE Datei

## Autor

GJD-01

## Referenzen

- [DFRobot SEN0366 Wiki](https://wiki.dfrobot.com/Infrared_Laser_Distance_Sensor_50m_80m_SKU_SEN0366)
- [SEN0366 User Manual PDF](https://github.com/DFRobotdl/DFROBOTDL/blob/main/SEN0366/SEN0366%20User's%20Manual.pdf)
- [DFRobot Product Page](https://www.dfrobot.com/product-2108.html)

## Changelog

### Version 1.0.0 (2024)
- Erste vollständige Implementierung
- Alle Protokoll-Kommandos implementiert
- M5Stack Core2 Support
- Drei vollständige Beispiele
- Umfassende Dokumentation

## Support

Bei Fragen oder Problemen:
- Öffnen Sie ein Issue auf GitHub
- Konsultieren Sie die Beispiele
- Aktivieren Sie den Debug-Modus für detaillierte Ausgaben

## Beiträge

Beiträge sind willkommen! Bitte öffnen Sie einen Pull Request oder ein Issue auf GitHub.
