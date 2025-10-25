/**
 * @file LaserRangefinder_SEN0366.cpp
 * @brief Implementation of the SEN0366 Laser Rangefinder library
 * @author GJD-01
 * @version 1.0.0
 */

#include "LaserRangefinder_SEN0366.h"

// ==================== Constructors ====================

LaserRangefinder_SEN0366::LaserRangefinder_SEN0366(HardwareSerial* serial)
    : _serial(serial), _address(SEN0366_BROADCAST_ADDR), _timeout(SEN0366_TIMEOUT_MS),
      _debug(false), _debugSerial(&Serial) {
}

LaserRangefinder_SEN0366::LaserRangefinder_SEN0366(HardwareSerial* serial, uint8_t address)
    : _serial(serial), _address(address), _timeout(SEN0366_TIMEOUT_MS),
      _debug(false), _debugSerial(&Serial) {
}

void LaserRangefinder_SEN0366::begin() {
    // Serial already initialized by user, just prepare
    delay(100); // Wait for serial to stabilize
    clearSerialBuffer();
}

void LaserRangefinder_SEN0366::begin(uint32_t baudRate) {
#if defined(ESP32)
    // For ESP32, we cannot set pins here - user must call begin(baudRate, rxPin, txPin)
    // or initialize Serial themselves before calling this
    if (_debug) {
        _debugSerial->println("WARNING: For ESP32, use begin(baudRate, rxPin, txPin) or initialize Serial manually!");
    }
#endif
    _serial->begin(baudRate);
    delay(100); // Wait for serial to stabilize
    clearSerialBuffer();
}

#if defined(ESP32)
void LaserRangefinder_SEN0366::begin(uint32_t baudRate, int8_t rxPin, int8_t txPin) {
    // ESP32-specific initialization with custom pins
    _serial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
    delay(100); // Wait for serial to stabilize
    clearSerialBuffer();
}
#endif

// ==================== Measurement Commands ====================

bool LaserRangefinder_SEN0366::singleMeasurement(float& distance) {
    // Command: <ADDR> 06 02 <CS>
    uint8_t cmd[4];
    cmd[0] = _address;
    cmd[1] = 0x06;
    cmd[2] = 0x02;
    cmd[3] = calculateChecksum(cmd, 3);

    if (_debug) {
        printHex(cmd, 4, "TX Single Measurement: ");
    }

    if (!sendCommand(cmd, 4)) {
        return false;
    }

    // Wait for response
    uint8_t response[20];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        if (_debug) _debugSerial->println("Single measurement: No response");
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Single Measurement: ");
    }

    return parseDistanceResponse(response, len, distance);
}

bool LaserRangefinder_SEN0366::startContinuousMeasurement() {
    // Command: <ADDR> 06 03 <CS>
    uint8_t cmd[4];
    cmd[0] = _address;
    cmd[1] = 0x06;
    cmd[2] = 0x03;
    cmd[3] = calculateChecksum(cmd, 3);

    if (_debug) {
        printHex(cmd, 4, "TX Start Continuous: ");
    }

    // Clear any old data before starting
    clearSerialBuffer();

    // Note: This command starts streaming data, no initial ACK expected
    if (!sendCommand(cmd, 4)) {
        return false;
    }

    // Give sensor time to start streaming
    delay(200);

    if (_debug) {
        _debugSerial->println("Continuous measurement started, waiting for data...");
    }

    return true;
}

bool LaserRangefinder_SEN0366::readContinuousDistance(float& distance) {
    // In continuous mode, device sends measurements automatically
    // Format: <ADDR> 06 82/83 ddd.ddd <CS> or <ADDR> 06 82/83 ddd.dddd <CS>

    // Wait briefly for data if not immediately available
    if (_serial->available() < 11) {
        unsigned long waitStart = millis();
        while (_serial->available() < 11 && (millis() - waitStart) < 50) {
            delay(5);
        }

        // If still no data after waiting, return false
        if (_serial->available() == 0) {
            return false;
        }
    }

    uint8_t response[20];
    uint8_t len = 0;

    // Read available data
    unsigned long startTime = millis();
    while (_serial->available() && len < sizeof(response) && (millis() - startTime < 100)) {
        response[len++] = _serial->read();

        // Check if we have a complete message
        if (len >= 11) {
            // Verify basic structure (accept both 0x82 and 0x83 for continuous measurements)
            if (response[0] == _address && response[1] == 0x06 && (response[2] == 0x82 || response[2] == 0x83)) {
                // Calculate expected checksum position
                // Message ends with <CS>, need to find the decimal point and digits
                uint8_t checksumPos = 0;
                for (uint8_t i = 3; i < len; i++) {
                    if (i > 3 && (response[i] < '0' || response[i] > '9') && response[i] != '.') {
                        checksumPos = i;
                        break;
                    }
                }

                if (checksumPos > 0 && checksumPos == len - 1) {
                    // We have a complete message
                    break;
                }
            }
        }

        // Small delay to let more data arrive
        if (_serial->available() == 0) {
            delay(5);
        }
    }

    if (len < 11) {
        if (_debug) {
            _debugSerial->print("Incomplete message, len=");
            _debugSerial->println(len);
        }
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Continuous: ");
    }

    return parseDistanceResponse(response, len, distance);
}

bool LaserRangefinder_SEN0366::stopContinuousMeasurement() {
    // Command: <ADDR> 06 05 <CS> (Control Laser Off)
    uint8_t cmd[4];
    cmd[0] = _address;
    cmd[1] = 0x06;
    cmd[2] = 0x05;
    cmd[3] = calculateChecksum(cmd, 3);

    if (_debug) {
        printHex(cmd, 4, "TX Stop Continuous (Laser Off): ");
    }

    if (!sendCommand(cmd, 4)) {
        return false;
    }

    // Wait for response
    uint8_t response[6];
    uint8_t len = 0;
    if (waitForResponse(response, sizeof(response), len)) {
        if (_debug) {
            printHex(response, len, "RX Stop Continuous: ");
        }

        // Expected: <ADDR> 06 85 00 <CS> (laser off confirmation)
        if (len >= 4 && response[0] == _address && response[1] == 0x06 && response[2] == 0x85) {
            // Clear any remaining measurement data from buffer
            clearSerialBuffer();
            return true;
        }
    }

    // Even if no response, clear buffer and return success
    // (some firmware versions may not send ACK)
    clearSerialBuffer();
    return true;
}

void LaserRangefinder_SEN0366::takeSingleMeasurementToCache() {
    // Command: FA 06 06 FA (broadcast only, no response)
    uint8_t cmd[4] = {0xFA, 0x06, 0x06, 0xFA};

    if (_debug) {
        printHex(cmd, 4, "TX Take Measurement to Cache: ");
    }

    sendCommand(cmd, 4);
}

bool LaserRangefinder_SEN0366::readCache(float& distance) {
    // Command: <ADDR> 06 07 <CS>
    uint8_t cmd[4];
    cmd[0] = _address;
    cmd[1] = 0x06;
    cmd[2] = 0x07;
    cmd[3] = calculateChecksum(cmd, 3);

    if (_debug) {
        printHex(cmd, 4, "TX Read Cache: ");
    }

    if (!sendCommand(cmd, 4)) {
        return false;
    }

    uint8_t response[20];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Read Cache: ");
    }

    return parseDistanceResponse(response, len, distance);
}

// ==================== Configuration Commands ====================

bool LaserRangefinder_SEN0366::setAddress(uint8_t newAddress) {
    // Command: FA 04 01 <ADDR> <CS>
    if (newAddress < SEN0366_MIN_ADDR || newAddress > SEN0366_MAX_ADDR) {
        if (_debug) _debugSerial->println("Invalid address");
        return false;
    }

    uint8_t cmd[5];
    cmd[0] = SEN0366_BROADCAST_ADDR;
    cmd[1] = 0x04;
    cmd[2] = 0x01;
    cmd[3] = newAddress;
    cmd[4] = calculateChecksum(cmd, 4);

    if (_debug) {
        printHex(cmd, 5, "TX Set Address: ");
    }

    if (!sendCommand(cmd, 5)) {
        return false;
    }

    uint8_t response[5];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Set Address: ");
    }

    // Expected: FA 01 81 81
    if (len == 4 && response[0] == 0xFA && response[1] == 0x01 && response[2] == 0x81) {
        _address = newAddress; // Update internal address
        return true;
    }

    return false;
}

bool LaserRangefinder_SEN0366::setResolution(uint8_t resolution) {
    // Command: FA 04 0C <RES> <CS>
    if (resolution != SEN0366_RESOLUTION_1MM && resolution != SEN0366_RESOLUTION_01MM) {
        if (_debug) _debugSerial->println("Invalid resolution");
        return false;
    }

    uint8_t cmd[5];
    cmd[0] = SEN0366_BROADCAST_ADDR;
    cmd[1] = 0x04;
    cmd[2] = 0x0C;
    cmd[3] = resolution;
    cmd[4] = calculateChecksum(cmd, 4);

    if (_debug) {
        printHex(cmd, 5, "TX Set Resolution: ");
    }

    if (!sendCommand(cmd, 5)) {
        return false;
    }

    uint8_t response[5];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Set Resolution: ");
    }

    // Expected: FA 04 8C 76 (success)
    return (len == 4 && response[0] == 0xFA && response[1] == 0x04 && response[2] == 0x8C);
}

bool LaserRangefinder_SEN0366::setFrequency(uint8_t frequency) {
    // Command: FA 04 0A <FRQ> <CS>
    uint8_t cmd[5];
    cmd[0] = SEN0366_BROADCAST_ADDR;
    cmd[1] = 0x04;
    cmd[2] = 0x0A;
    cmd[3] = frequency;
    cmd[4] = calculateChecksum(cmd, 4);

    if (_debug) {
        printHex(cmd, 5, "TX Set Frequency: ");
    }

    if (!sendCommand(cmd, 5)) {
        return false;
    }

    uint8_t response[5];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Set Frequency: ");
    }

    // Expected: FA 04 8A 78 (success)
    return (len == 4 && response[0] == 0xFA && response[1] == 0x04 && response[2] == 0x8A);
}

bool LaserRangefinder_SEN0366::setDataReturnInterval(uint8_t interval) {
    // Command: FA 04 05 <INT> <CS>
    uint8_t cmd[5];
    cmd[0] = SEN0366_BROADCAST_ADDR;
    cmd[1] = 0x04;
    cmd[2] = 0x05;
    cmd[3] = interval;
    cmd[4] = calculateChecksum(cmd, 4);

    if (_debug) {
        printHex(cmd, 5, "TX Set Data Return Interval: ");
    }

    if (!sendCommand(cmd, 5)) {
        return false;
    }

    uint8_t response[5];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Set Data Return Interval: ");
    }

    // Expected: FA 04 85 7D (success)
    return (len == 4 && response[0] == 0xFA && response[1] == 0x04 && response[2] == 0x85);
}

bool LaserRangefinder_SEN0366::setMeasurementStartingPoint(uint8_t position) {
    // Command: FA 04 08 <POS> <CS>
    uint8_t cmd[5];
    cmd[0] = SEN0366_BROADCAST_ADDR;
    cmd[1] = 0x04;
    cmd[2] = 0x08;
    cmd[3] = position;
    cmd[4] = calculateChecksum(cmd, 4);

    if (_debug) {
        printHex(cmd, 5, "TX Set Measurement Starting Point: ");
    }

    if (!sendCommand(cmd, 5)) {
        return false;
    }

    uint8_t response[5];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Set Measurement Starting Point: ");
    }

    // Expected: FA 04 88 7A (success)
    return (len == 4 && response[0] == 0xFA && response[1] == 0x04 && response[2] == 0x88);
}

bool LaserRangefinder_SEN0366::setAutoStart(uint8_t enable) {
    // Command: FA 04 0D <AUTO> <CS>
    uint8_t cmd[5];
    cmd[0] = SEN0366_BROADCAST_ADDR;
    cmd[1] = 0x04;
    cmd[2] = 0x0D;
    cmd[3] = enable;
    cmd[4] = calculateChecksum(cmd, 4);

    if (_debug) {
        printHex(cmd, 5, "TX Set Auto Start: ");
    }

    if (!sendCommand(cmd, 5)) {
        return false;
    }

    uint8_t response[5];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Set Auto Start: ");
    }

    // Expected: FA 04 8D 75 (success)
    return (len == 4 && response[0] == 0xFA && response[1] == 0x04 && response[2] == 0x8D);
}

bool LaserRangefinder_SEN0366::reviseDistance(bool positive, uint8_t adjustment) {
    // Command: FA 04 06 {+|-} <ADJ> FF
    // Note: Documentation shows inconsistencies, using shown format
    uint8_t cmd[5];
    cmd[0] = SEN0366_BROADCAST_ADDR;
    cmd[1] = 0x04;
    cmd[2] = 0x06;
    cmd[3] = positive ? 0x2B : 0x2D; // '+' or '-'
    cmd[4] = adjustment;
    // Note: Documentation shows 0xFF as checksum - needs verification

    if (_debug) {
        printHex(cmd, 5, "TX Revise Distance: ");
    }

    if (!sendCommand(cmd, 5)) {
        return false;
    }

    uint8_t response[5];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Revise Distance: ");
    }

    // Expected: FA 04 8B 77 (per documentation, though irregular)
    return (len == 4 && response[0] == 0xFA && response[1] == 0x04 && response[2] == 0x8B);
}

// ==================== Control Commands ====================

bool LaserRangefinder_SEN0366::controlLaser(uint8_t on) {
    // Command: <ADDR> 06 05 <CS>
    // Note: The on/off control seems to be by command presence, not parameter
    // Based on doc: 06 05 is the command, on/off might be in different message
    // Using address-specific command per protocol
    uint8_t cmd[4];
    cmd[0] = _address;
    cmd[1] = 0x06;
    cmd[2] = 0x05;
    cmd[3] = calculateChecksum(cmd, 3);

    if (_debug) {
        printHex(cmd, 4, "TX Control Laser: ");
    }

    if (!sendCommand(cmd, 4)) {
        return false;
    }

    uint8_t response[6];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Control Laser: ");
    }

    // Expected: <ADDR> 06 85 01 <CS> or <ADDR> 06 85 00 <CS>
    return (len >= 4 && response[0] == _address && response[1] == 0x06 && response[2] == 0x85);
}

bool LaserRangefinder_SEN0366::shutDown() {
    // Command: <ADDR> 04 02 <CS>
    uint8_t cmd[4];
    cmd[0] = _address;
    cmd[1] = 0x04;
    cmd[2] = 0x02;
    cmd[3] = calculateChecksum(cmd, 3);

    if (_debug) {
        printHex(cmd, 4, "TX Shut Down: ");
    }

    // No response expected per documentation
    return sendCommand(cmd, 4);
}

// ==================== Information Commands ====================

uint8_t LaserRangefinder_SEN0366::readParameter(uint8_t* data, uint8_t maxLen) {
    // Command: FA 06 01 FF
    uint8_t cmd[4] = {0xFA, 0x06, 0x01, 0xFF};

    if (_debug) {
        printHex(cmd, 4, "TX Read Parameter: ");
    }

    if (!sendCommand(cmd, 4)) {
        return 0;
    }

    uint8_t len = 0;
    if (!waitForResponse(data, maxLen, len)) {
        return 0;
    }

    if (_debug) {
        printHex(data, len, "RX Read Parameter: ");
    }

    return len;
}

bool LaserRangefinder_SEN0366::readMachineNumber(char* serialNumber) {
    // Command: FA 06 04 FC
    uint8_t cmd[4] = {0xFA, 0x06, 0x04, 0xFC};

    if (_debug) {
        printHex(cmd, 4, "TX Read Machine Number: ");
    }

    if (!sendCommand(cmd, 4)) {
        return false;
    }

    uint8_t response[25];
    uint8_t len = 0;
    if (!waitForResponse(response, sizeof(response), len)) {
        return false;
    }

    if (_debug) {
        printHex(response, len, "RX Read Machine Number: ");
    }

    // Expected: FA 06 84 <16 bytes serial> <CS>
    if (len >= 20 && response[0] == 0xFA && response[1] == 0x06 && response[2] == 0x84) {
        memcpy(serialNumber, &response[3], 16);
        serialNumber[16] = '\0';
        return true;
    }

    return false;
}

// ==================== Helper Functions ====================

void LaserRangefinder_SEN0366::setDeviceAddress(uint8_t address) {
    _address = address;
}

uint8_t LaserRangefinder_SEN0366::getDeviceAddress() const {
    return _address;
}

bool LaserRangefinder_SEN0366::isBroadcast() const {
    return (_address == SEN0366_BROADCAST_ADDR);
}

void LaserRangefinder_SEN0366::setTimeout(uint32_t timeoutMs) {
    _timeout = timeoutMs;
}

void LaserRangefinder_SEN0366::setDebug(bool enable, Stream* debugSerial) {
    _debug = enable;
    _debugSerial = debugSerial;
}

// ==================== Private Helper Functions ====================

uint8_t LaserRangefinder_SEN0366::calculateChecksum(const uint8_t* data, uint8_t len) {
    // Checksum: CS = ~SUM + 1 (two's complement)
    uint16_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)((~sum + 1) & 0xFF);
}

bool LaserRangefinder_SEN0366::sendCommand(const uint8_t* cmd, uint8_t len) {
    if (!_serial) return false;

    clearSerialBuffer(); // Clear any old data

    size_t written = _serial->write(cmd, len);
    _serial->flush();

    return (written == len);
}

uint8_t LaserRangefinder_SEN0366::receiveResponse(uint8_t* buffer, uint8_t maxLen) {
    uint8_t len = 0;
    while (_serial->available() && len < maxLen) {
        buffer[len++] = _serial->read();
    }
    return len;
}

bool LaserRangefinder_SEN0366::waitForResponse(uint8_t* buffer, uint8_t maxLen, uint8_t& len) {
    unsigned long startTime = millis();
    len = 0;

    while (millis() - startTime < _timeout) {
        if (_serial->available()) {
            buffer[len++] = _serial->read();

            if (len >= maxLen) {
                break;
            }

            // Wait a bit more for complete message
            delay(10);

            // If no more data coming, we're done
            if (!_serial->available()) {
                delay(5);
                if (!_serial->available()) {
                    break;
                }
            }
        }
    }

    return (len > 0);
}

bool LaserRangefinder_SEN0366::parseDistanceResponse(const uint8_t* response, uint8_t len, float& distance) {
    // Expected format: <ADDR> 06 82 ddd.ddd <CS> or <ADDR> 06 82 ddd.dddd <CS>
    // Or error: <ADDR> 06 82 ERR---dd <CS>

    if (len < 11) {
        if (_debug) _debugSerial->println("Response too short");
        return false;
    }

    // Verify header
    // Accept 0x82, 0x83 (continuous measurement), 0x87 (single measurement)
    if (response[1] != 0x06 || (response[2] != 0x82 && response[2] != 0x83 && response[2] != 0x87)) {
        if (_debug) {
            _debugSerial->print("Invalid response header: ");
            _debugSerial->print(response[1], HEX);
            _debugSerial->print(" ");
            _debugSerial->println(response[2], HEX);
        }
        return false;
    }

    // Check for error message
    if (response[3] == 'E' && response[4] == 'R' && response[5] == 'R') {
        if (_debug) _debugSerial->println("Error response from device");
        return false;
    }

    // Extract distance string
    char distStr[12] = {0};
    uint8_t distLen = 0;
    for (uint8_t i = 3; i < len - 1 && distLen < 11; i++) {
        if ((response[i] >= '0' && response[i] <= '9') || response[i] == '.') {
            distStr[distLen++] = (char)response[i];
        } else {
            break;
        }
    }
    distStr[distLen] = '\0';

    if (distLen == 0) {
        if (_debug) _debugSerial->println("No distance data found");
        return false;
    }

    // Convert to float (sensor returns value in METERS)
    distance = atof(distStr);

    if (_debug) {
        _debugSerial->print("Parsed distance: ");
        _debugSerial->print(distance);
        _debugSerial->println(" m");
    }

    return true;
}

void LaserRangefinder_SEN0366::printHex(const uint8_t* data, uint8_t len, const char* prefix) {
    if (!_debug || !_debugSerial) return;

    if (prefix) {
        _debugSerial->print(prefix);
    }

    for (uint8_t i = 0; i < len; i++) {
        if (data[i] < 0x10) _debugSerial->print("0");
        _debugSerial->print(data[i], HEX);
        _debugSerial->print(" ");
    }
    _debugSerial->println();
}

void LaserRangefinder_SEN0366::clearSerialBuffer() {
    while (_serial->available()) {
        _serial->read();
    }
}
