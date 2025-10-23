/**
 * @file LaserRangefinder_SEN0366.h
 * @brief Arduino library for DFRobot SEN0366 Laser Rangefinder
 * @author GJD-01
 * @version 1.0.0
 *
 * This library provides complete support for the SEN0366 Laser Rangefinder
 * including all UART protocol commands, single and continuous measurements,
 * and support for both broadcast and individual device addressing.
 *
 * Protocol: UART 9600 baud, 8N1, 3.3V TTL
 * Broadcast Address: 0xFA
 * Individual Addresses: 0x80 to 0xF9
 */

#ifndef LASERRANGEFINDER_SEN0366_H
#define LASERRANGEFINDER_SEN0366_H

#include <Arduino.h>

// Protocol Constants
#define SEN0366_BROADCAST_ADDR  0xFA
#define SEN0366_MIN_ADDR        0x80
#define SEN0366_MAX_ADDR        0xF9
#define SEN0366_BAUD_RATE       9600

// Resolution Settings
#define SEN0366_RESOLUTION_1MM   0x01
#define SEN0366_RESOLUTION_01MM  0x02

// Frequency Settings
#define SEN0366_FREQ_0HZ   0x00
#define SEN0366_FREQ_5HZ   0x05
#define SEN0366_FREQ_10HZ  0x0A
#define SEN0366_FREQ_20HZ  0x14

// Measurement Starting Point
#define SEN0366_MEASURE_FROM_FRONT  0x01
#define SEN0366_MEASURE_FROM_REAR   0x00

// Auto Start Settings
#define SEN0366_AUTOSTART_DISABLE  0x00
#define SEN0366_AUTOSTART_ENABLE   0x01

// Laser Control
#define SEN0366_LASER_OFF  0x00
#define SEN0366_LASER_ON   0x01

// Command Response Timeout (milliseconds)
#define SEN0366_TIMEOUT_MS  1000

// Maximum distance string length
#define SEN0366_MAX_DIST_STR_LEN  12

/**
 * @brief Main class for controlling the SEN0366 Laser Rangefinder
 */
class LaserRangefinder_SEN0366 {
public:
    /**
     * @brief Constructor for broadcast mode (single rangefinder)
     * @param serial Pointer to HardwareSerial object
     */
    LaserRangefinder_SEN0366(HardwareSerial* serial);

    /**
     * @brief Constructor for individual addressing (multiple rangefinders)
     * @param serial Pointer to HardwareSerial object
     * @param address Device address (0x80 to 0xF9)
     */
    LaserRangefinder_SEN0366(HardwareSerial* serial, uint8_t address);

    /**
     * @brief Initialize the rangefinder
     * @param baudRate Baud rate (default: 9600)
     */
    void begin(uint32_t baudRate = SEN0366_BAUD_RATE);

    // ==================== Measurement Commands ====================

    /**
     * @brief Perform a single distance measurement
     * @param distance Output parameter for distance in mm
     * @return true if successful, false otherwise
     */
    bool singleMeasurement(float& distance);

    /**
     * @brief Start continuous measurement mode
     * @return true if successful, false otherwise
     */
    bool startContinuousMeasurement();

    /**
     * @brief Read the next distance value in continuous mode
     * @param distance Output parameter for distance in mm
     * @return true if data available and valid, false otherwise
     */
    bool readContinuousDistance(float& distance);

    /**
     * @brief Stop continuous measurement mode (send laser off command)
     * @return true if successful, false otherwise
     */
    bool stopContinuousMeasurement();

    /**
     * @brief Take a single measurement and store in cache (for synchronized multi-device readings)
     * Broadcast only - does not return acknowledgment
     */
    void takeSingleMeasurementToCache();

    /**
     * @brief Read the cached measurement value
     * @param distance Output parameter for distance in mm
     * @return true if successful, false otherwise
     */
    bool readCache(float& distance);

    // ==================== Configuration Commands ====================

    /**
     * @brief Set device address (use only with single device connected)
     * @param newAddress New address (0x80 to 0xF9)
     * @return true if successful, false otherwise
     */
    bool setAddress(uint8_t newAddress);

    /**
     * @brief Set measurement resolution
     * @param resolution SEN0366_RESOLUTION_1MM or SEN0366_RESOLUTION_01MM
     * @return true if successful, false otherwise
     */
    bool setResolution(uint8_t resolution);

    /**
     * @brief Set measurement frequency
     * @param frequency SEN0366_FREQ_0HZ, _5HZ, _10HZ, or _20HZ
     * @return true if successful, false otherwise
     */
    bool setFrequency(uint8_t frequency);

    /**
     * @brief Set data return interval for continuous mode
     * @param interval Interval value
     * @return true if successful, false otherwise
     */
    bool setDataReturnInterval(uint8_t interval);

    /**
     * @brief Set measurement starting point (reference zero)
     * @param position SEN0366_MEASURE_FROM_FRONT or SEN0366_MEASURE_FROM_REAR
     * @return true if successful, false otherwise
     */
    bool setMeasurementStartingPoint(uint8_t position);

    /**
     * @brief Enable or disable automatic measurement start on power-up
     * @param enable SEN0366_AUTOSTART_ENABLE or SEN0366_AUTOSTART_DISABLE
     * @return true if successful, false otherwise
     */
    bool setAutoStart(uint8_t enable);

    /**
     * @brief Adjust distance measurement offset
     * @param positive true for positive adjustment, false for negative
     * @param adjustment Adjustment value
     * @return true if successful, false otherwise
     */
    bool reviseDistance(bool positive, uint8_t adjustment);

    // ==================== Control Commands ====================

    /**
     * @brief Control laser on/off
     * @param on SEN0366_LASER_ON or SEN0366_LASER_OFF
     * @return true if successful, false otherwise
     */
    bool controlLaser(uint8_t on);

    /**
     * @brief Shut down the rangefinder
     * @return true if successful, false otherwise
     */
    bool shutDown();

    // ==================== Information Commands ====================

    /**
     * @brief Read device parameters
     * @param data Buffer to store parameter data
     * @param maxLen Maximum length of buffer
     * @return Number of bytes read, 0 on error
     */
    uint8_t readParameter(uint8_t* data, uint8_t maxLen);

    /**
     * @brief Read machine serial number
     * @param serialNumber Buffer for 16-byte serial number (will be null-terminated)
     * @return true if successful, false otherwise
     */
    bool readMachineNumber(char* serialNumber);

    // ==================== Helper Functions ====================

    /**
     * @brief Set the device address for this instance
     * @param address New address to use
     */
    void setDeviceAddress(uint8_t address);

    /**
     * @brief Get current device address
     * @return Current address
     */
    uint8_t getDeviceAddress() const;

    /**
     * @brief Check if using broadcast mode
     * @return true if broadcast, false if individual address
     */
    bool isBroadcast() const;

    /**
     * @brief Set timeout for command responses
     * @param timeoutMs Timeout in milliseconds
     */
    void setTimeout(uint32_t timeoutMs);

    /**
     * @brief Enable or disable debug output
     * @param enable true to enable debug, false to disable
     * @param debugSerial Serial port for debug output (default: Serial)
     */
    void setDebug(bool enable, Stream* debugSerial = &Serial);

private:
    HardwareSerial* _serial;
    uint8_t _address;
    uint32_t _timeout;
    bool _debug;
    Stream* _debugSerial;

    // Internal helper functions
    uint8_t calculateChecksum(const uint8_t* data, uint8_t len);
    bool sendCommand(const uint8_t* cmd, uint8_t len);
    uint8_t receiveResponse(uint8_t* buffer, uint8_t maxLen);
    bool waitForResponse(uint8_t* buffer, uint8_t maxLen, uint8_t& len);
    bool parseDistanceResponse(const uint8_t* response, uint8_t len, float& distance);
    void printHex(const uint8_t* data, uint8_t len, const char* prefix = nullptr);
    void clearSerialBuffer();
};

#endif // LASERRANGEFINDER_SEN0366_H
