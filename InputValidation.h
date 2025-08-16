// ============================================================================
// File: InputValidation.h
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Description: Centralized input validation and bounds checking utilities
// ============================================================================

#ifndef INPUT_VALIDATION_H
#define INPUT_VALIDATION_H

#include <Arduino.h>
#include <limits.h>
#include <float.h>

// ============================================================================
// Parameter Limits - Centralized definition of all system limits
// ============================================================================

namespace ParamLimits {
    // Motion parameters
    constexpr float MIN_SPEED = 1.0f;           // Minimum speed to prevent stalling
    constexpr float MAX_SPEED = 20000.0f;       // Maximum safe speed
    constexpr float MIN_ACCELERATION = 1.0f;    // Minimum acceleration
    constexpr float MAX_ACCELERATION = 20000.0f; // Maximum safe acceleration
    constexpr float MIN_JERK = 0.0f;            // Minimum jerk
    constexpr float MAX_JERK = 50000.0f;        // Maximum jerk
    
    // Position parameters
    constexpr int32_t MIN_POSITION = -2000000;   // Minimum position (-2M steps)
    constexpr int32_t MAX_POSITION = 2000000;    // Maximum position (2M steps)
    constexpr float MIN_HOME_PERCENT = 0.0f;     // Minimum home position percentage
    constexpr float MAX_HOME_PERCENT = 100.0f;   // Maximum home position percentage
    
    // Homing parameters
    constexpr float MIN_HOMING_SPEED = 10.0f;    // Minimum homing speed
    constexpr float MAX_HOMING_SPEED = 10000.0f; // Maximum homing speed
    constexpr float MIN_LIMIT_MARGIN = 0.0f;     // Minimum limit safety margin
    constexpr float MAX_LIMIT_MARGIN = 10000.0f; // Maximum limit safety margin
    
    // DMX parameters
    constexpr uint16_t MIN_DMX_CHANNEL = 1;      // Minimum DMX channel
    constexpr uint16_t MAX_DMX_CHANNEL = 508;    // Maximum DMX start channel (508 + 5 channels = 512)
    constexpr uint32_t MIN_DMX_TIMEOUT = 100;    // Minimum DMX timeout (ms)
    constexpr uint32_t MAX_DMX_TIMEOUT = 60000;  // Maximum DMX timeout (ms)
    constexpr float MIN_DMX_SCALE = -1000.0f;    // Minimum DMX scale factor
    constexpr float MAX_DMX_SCALE = 1000.0f;     // Maximum DMX scale factor
    
    // Safety parameters
    constexpr float MIN_EMERGENCY_DECEL = 100.0f;   // Minimum emergency deceleration
    constexpr float MAX_EMERGENCY_DECEL = 50000.0f; // Maximum emergency deceleration
    
    // System parameters
    constexpr uint32_t MIN_STATUS_INTERVAL = 10;    // Minimum status update interval (ms)
    constexpr uint32_t MAX_STATUS_INTERVAL = 10000; // Maximum status update interval (ms)
    constexpr uint8_t MIN_VERBOSITY = 0;            // Minimum verbosity level
    constexpr uint8_t MAX_VERBOSITY = 3;            // Maximum verbosity level
}

// ============================================================================
// Input Validation Functions
// ============================================================================

namespace InputValidation {
    
    /**
     * Validate and constrain an integer value
     * @param value The value to validate
     * @param minVal Minimum allowed value
     * @param maxVal Maximum allowed value
     * @param paramName Name of parameter for error reporting
     * @return true if value was already valid, false if it was clamped
     */
    inline bool validateInt32(int32_t& value, int32_t minVal, int32_t maxVal, const char* paramName = nullptr) {
        if (value < minVal || value > maxVal) {
            int32_t original = value;
            value = constrain(value, minVal, maxVal);
            if (paramName) {
                Serial.printf("[VALIDATION] WARNING: %s value %d out of range [%d, %d], clamped to %d\n",
                             paramName, original, minVal, maxVal, value);
            }
            return false;
        }
        return true;
    }
    
    /**
     * Validate and constrain a float value
     * @param value The value to validate
     * @param minVal Minimum allowed value
     * @param maxVal Maximum allowed value
     * @param paramName Name of parameter for error reporting
     * @return true if value was already valid, false if it was clamped
     */
    inline bool validateFloat(float& value, float minVal, float maxVal, const char* paramName = nullptr) {
        // Check for NaN or infinity
        if (isnan(value) || isinf(value)) {
            value = (minVal + maxVal) / 2.0f; // Default to midpoint
            if (paramName) {
                Serial.printf("[VALIDATION] ERROR: %s is NaN or infinite, set to %.2f\n", paramName, value);
            }
            return false;
        }
        
        if (value < minVal || value > maxVal) {
            float original = value;
            value = constrain(value, minVal, maxVal);
            if (paramName) {
                Serial.printf("[VALIDATION] WARNING: %s value %.2f out of range [%.2f, %.2f], clamped to %.2f\n",
                             paramName, original, minVal, maxVal, value);
            }
            return false;
        }
        return true;
    }
    
    /**
     * Validate a non-zero float value
     * @param value The value to validate
     * @param paramName Name of parameter for error reporting
     * @return true if value is non-zero, false otherwise
     */
    inline bool validateNonZero(float& value, const char* paramName = nullptr) {
        if (value == 0.0f) {
            value = 1.0f; // Default to 1
            if (paramName) {
                Serial.printf("[VALIDATION] ERROR: %s cannot be zero, set to 1.0\n", paramName);
            }
            return false;
        }
        return true;
    }
    
    /**
     * Parse and validate integer from string
     * @param str Input string
     * @param value Output value
     * @param minVal Minimum allowed value
     * @param maxVal Maximum allowed value
     * @param paramName Name of parameter for error reporting
     * @return true if parse and validation successful
     */
    inline bool parseAndValidateInt(const char* str, int32_t& value, int32_t minVal, int32_t maxVal, 
                                    const char* paramName = nullptr) {
        if (!str || *str == '\0') {
            if (paramName) {
                Serial.printf("[VALIDATION] ERROR: %s is empty or null\n", paramName);
            }
            return false;
        }
        
        char* endPtr;
        errno = 0;
        long parsed = strtol(str, &endPtr, 10);
        
        // Check for parsing errors
        if (errno == ERANGE || parsed < INT32_MIN || parsed > INT32_MAX) {
            if (paramName) {
                Serial.printf("[VALIDATION] ERROR: %s value '%s' out of int32 range\n", paramName, str);
            }
            return false;
        }
        
        if (endPtr == str || *endPtr != '\0') {
            if (paramName) {
                Serial.printf("[VALIDATION] ERROR: %s value '%s' is not a valid integer\n", paramName, str);
            }
            return false;
        }
        
        value = (int32_t)parsed;
        return validateInt32(value, minVal, maxVal, paramName);
    }
    
    /**
     * Parse and validate float from string
     * @param str Input string
     * @param value Output value
     * @param minVal Minimum allowed value
     * @param maxVal Maximum allowed value
     * @param paramName Name of parameter for error reporting
     * @return true if parse and validation successful
     */
    inline bool parseAndValidateFloat(const char* str, float& value, float minVal, float maxVal,
                                      const char* paramName = nullptr) {
        if (!str || *str == '\0') {
            if (paramName) {
                Serial.printf("[VALIDATION] ERROR: %s is empty or null\n", paramName);
            }
            return false;
        }
        
        char* endPtr;
        errno = 0;
        float parsed = strtof(str, &endPtr);
        
        // Check for parsing errors
        if (errno == ERANGE) {
            if (paramName) {
                Serial.printf("[VALIDATION] ERROR: %s value '%s' out of float range\n", paramName, str);
            }
            return false;
        }
        
        if (endPtr == str || *endPtr != '\0') {
            if (paramName) {
                Serial.printf("[VALIDATION] ERROR: %s value '%s' is not a valid float\n", paramName, str);
            }
            return false;
        }
        
        value = parsed;
        return validateFloat(value, minVal, maxVal, paramName);
    }
    
    /**
     * Validate motion profile parameters
     * @param speed Speed value to validate
     * @param accel Acceleration value to validate
     * @param jerk Jerk value to validate (optional)
     * @return true if all parameters are valid
     */
    inline bool validateMotionProfile(float& speed, float& accel, float* jerk = nullptr) {
        bool valid = true;
        valid &= validateFloat(speed, ParamLimits::MIN_SPEED, ParamLimits::MAX_SPEED, "speed");
        valid &= validateFloat(accel, ParamLimits::MIN_ACCELERATION, ParamLimits::MAX_ACCELERATION, "acceleration");
        if (jerk) {
            valid &= validateFloat(*jerk, ParamLimits::MIN_JERK, ParamLimits::MAX_JERK, "jerk");
        }
        return valid;
    }
    
    /**
     * Validate position value
     * @param position Position value to validate
     * @param useUserLimits Whether to use user-configured limits
     * @param userMin User minimum position (if useUserLimits is true)
     * @param userMax User maximum position (if useUserLimits is true)
     * @return true if position is valid
     */
    inline bool validatePosition(int32_t& position, bool useUserLimits = false, 
                                 int32_t userMin = 0, int32_t userMax = 0) {
        if (useUserLimits && userMin < userMax) {
            return validateInt32(position, userMin, userMax, "position");
        } else {
            return validateInt32(position, ParamLimits::MIN_POSITION, ParamLimits::MAX_POSITION, "position");
        }
    }
}

#endif // INPUT_VALIDATION_H