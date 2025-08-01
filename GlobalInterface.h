// ============================================================================
// File: GlobalInterface.h - Complete Thread-Safe Interface
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.0.0
// Date: 2025-01-23
// Author: Tim Rosener
// Description: Complete global data structures, enums, and thread-safe interfaces
// License: MIT
// ============================================================================

#ifndef GLOBALINTERFACE_H
#define GLOBALINTERFACE_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <atomic>

// ============================================================================
// Global Data Structures and Interfaces for SkullStepperV4
// ============================================================================

// ----------------------------------------------------------------------------
// System Enums
// ----------------------------------------------------------------------------
enum class SystemState {
  UNINITIALIZED,
  INITIALIZING,
  READY,
  RUNNING,
  STOPPING,
  STOPPED,
  ERROR,
  EMERGENCY_STOP
};

enum class MotionState {
  IDLE,
  ACCELERATING,
  CONSTANT_VELOCITY,
  DECELERATING,
  HOMING,
  POSITION_HOLD
};

enum class SafetyState {
  NORMAL,
  LEFT_LIMIT_ACTIVE,
  RIGHT_LIMIT_ACTIVE,
  BOTH_LIMITS_ACTIVE,
  STEPPER_ALARM,
  EMERGENCY_STOP,
  POSITION_ERROR
};

enum class DMXState {
  NO_SIGNAL,
  SIGNAL_PRESENT,
  TIMEOUT,
  ERROR
};

enum class CommandType {
  MOVE_ABSOLUTE,
  MOVE_RELATIVE,
  SET_SPEED,
  SET_ACCELERATION,
  HOME,
  STOP,
  EMERGENCY_STOP,
  ENABLE,
  DISABLE
};

// ----------------------------------------------------------------------------
// Motion Profile Structure
// ----------------------------------------------------------------------------
struct MotionProfile {
  float maxSpeed;           // Maximum speed (steps/sec)
  float acceleration;       // Acceleration (steps/sec²)
  float deceleration;       // Deceleration (steps/sec²)
  float jerk;              // Jerk limitation (steps/sec³)
  int32_t targetPosition;   // Target position (steps)
  bool enableLimits;        // Respect limit switches
};

// ----------------------------------------------------------------------------
// Motion Command Structure
// ----------------------------------------------------------------------------
struct MotionCommand {
  CommandType type;
  MotionProfile profile;
  uint32_t timestamp;
  uint16_t commandId;
};

// ----------------------------------------------------------------------------
// System Status Structure
// ----------------------------------------------------------------------------
struct SystemStatus {
  SystemState systemState;
  MotionState motionState;
  SafetyState safetyState;
  DMXState dmxState;
  
  int32_t currentPosition;
  int32_t targetPosition;
  float currentSpeed;
  
  bool stepperEnabled;
  bool limitsActive[2];     // [LEFT, RIGHT]
  bool stepperAlarm;
  
  uint16_t dmxChannel;
  uint16_t dmxValue;
  uint32_t lastDMXUpdate;
  
  uint32_t uptime;
  uint16_t errorCode;
};

// ----------------------------------------------------------------------------
// Configuration Structure
// ----------------------------------------------------------------------------
struct SystemConfig {
  // Motion Parameters
  MotionProfile defaultProfile;
  int32_t homePosition;
  int32_t minPosition;
  int32_t maxPosition;
  float homingSpeed;        // Speed for homing sequence (steps/sec)
  
  // DMX Settings
  uint16_t dmxStartChannel;
  float dmxScale;           // DMX to position scaling
  int32_t dmxOffset;        // DMX position offset
  uint32_t dmxTimeout;      // DMX timeout (ms)
  
  // Safety Settings
  bool enableLimitSwitches;
  bool enableStepperAlarm;
  float emergencyDeceleration;
  
  // System Settings
  uint32_t statusUpdateInterval;
  bool enableSerialOutput;
  uint8_t serialVerbosity;
  
  // Validation
  uint32_t configVersion;
  uint16_t checksum;
};

// ----------------------------------------------------------------------------
// Global Shared Data (Protected by Mutexes) - DEFINED IN GlobalInfrastructure.cpp
// ----------------------------------------------------------------------------
extern SystemStatus g_systemStatus;
extern SystemConfig g_systemConfig;
extern SemaphoreHandle_t g_statusMutex;
extern SemaphoreHandle_t g_configMutex;

// ----------------------------------------------------------------------------
// Inter-Module Communication Queues - DEFINED IN GlobalInfrastructure.cpp
// ----------------------------------------------------------------------------
extern QueueHandle_t g_motionCommandQueue;
extern QueueHandle_t g_statusUpdateQueue;
extern QueueHandle_t g_dmxDataQueue;

// ----------------------------------------------------------------------------
// Thread-Safe Access Macros
// ----------------------------------------------------------------------------
#define SAFE_READ_STATUS(field, dest) \
  do { \
    if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(10)) == pdTRUE) { \
      dest = g_systemStatus.field; \
      xSemaphoreGive(g_statusMutex); \
    } \
  } while(0)

#define SAFE_WRITE_STATUS(field, value) \
  do { \
    if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(10)) == pdTRUE) { \
      g_systemStatus.field = value; \
      xSemaphoreGive(g_statusMutex); \
    } \
  } while(0)

#define SAFE_READ_CONFIG(field, dest) \
  do { \
    if (xSemaphoreTake(g_configMutex, pdMS_TO_TICKS(10)) == pdTRUE) { \
      dest = g_systemConfig.field; \
      xSemaphoreGive(g_configMutex); \
    } \
  } while(0)

#define SAFE_WRITE_CONFIG(field, value) \
  do { \
    if (xSemaphoreTake(g_configMutex, pdMS_TO_TICKS(10)) == pdTRUE) { \
      g_systemConfig.field = value; \
      xSemaphoreGive(g_configMutex); \
    } \
  } while(0)

// ----------------------------------------------------------------------------
// Global Infrastructure Functions - IMPLEMENTED IN GlobalInfrastructure.cpp
// ----------------------------------------------------------------------------

/**
 * Initialize all global infrastructure with thread safety
 * Must be called before any other system modules
 * @return true if initialization successful
 */
bool initializeGlobalInfrastructure();

/**
 * Clean shutdown of global infrastructure
 * Safely deletes all FreeRTOS objects
 */
void shutdownGlobalInfrastructure();

/**
 * Validate system infrastructure integrity (thread-safe)
 * @return true if all infrastructure is healthy
 */
bool validateSystemIntegrity();

/**
 * Print complete infrastructure status for debugging
 */
void printInfrastructureStatus();

/**
 * Update system uptime in status structure (thread-safe)
 */
void updateSystemUptime();

/**
 * Broadcast system status update to monitoring queue (thread-safe)
 * @return true if status update queued successfully
 */
bool broadcastStatusUpdate();

/**
 * Get memory usage statistics (thread-safe)
 * @param freeHeap returns free heap memory in bytes
 * @param minFreeHeap returns minimum free heap since boot
 * @return true if statistics retrieved successfully
 */
bool getMemoryStats(uint32_t& freeHeap, uint32_t& minFreeHeap);

// ----------------------------------------------------------------------------
// Thread-Safe Utility Functions - IMPLEMENTED IN GlobalInfrastructure.cpp
// ----------------------------------------------------------------------------

/**
 * Get system uptime in milliseconds (thread-safe)
 * @return uptime since system start in milliseconds
 */
uint32_t getSystemUptime();

/**
 * Set system state with thread safety
 * @param newState new system state to set
 */
void setSystemState(SystemState newState);

/**
 * Get current system state with thread safety
 * @return current system state
 */
SystemState getSystemState();

/**
 * Calculate 16-bit checksum for data integrity (thread-safe)
 * @param data pointer to data to checksum
 * @param length length of data in bytes
 * @return 16-bit checksum
 */
uint16_t calculateChecksum(const void* data, size_t length);

// ----------------------------------------------------------------------------
// Module Interface Definitions (Implementation in respective modules)
// ----------------------------------------------------------------------------

// StepperController Interface
namespace StepperController {
  bool initialize();
  bool update();
  bool processMotionCommand(const MotionCommand& cmd);
  bool emergencyStop();
  bool enable(bool state);
  int32_t getCurrentPosition();
  float getCurrentSpeed();
  MotionState getMotionState();
  bool isMoving();
  MotionProfile getMotionProfile();
  bool setMotionProfile(const MotionProfile& profile);
  bool isEnabled();
  float getStepFrequency();
  bool getTimingDiagnostics(uint32_t& stepInterval, float& dutyCycle);
  
  // Homing and limit functions
  bool startHoming();
  bool isHoming();
  uint8_t getHomingProgress();
  bool isHomed();
  bool getPositionLimits(int32_t& minPos, int32_t& maxPos);
  void getLimitStates(bool& leftLimit, bool& rightLimit);
  
  // Advanced motion functions
  bool moveTo(int32_t position);
  bool move(int32_t steps);
  bool stop();
  bool setMaxSpeed(float speed);
  bool setAcceleration(float accel);
  int32_t distanceToGo();
  bool isAlarmActive();
}

// DMXReceiver Interface
namespace DMXReceiver {
  bool initialize();
  bool update();
  bool isSignalPresent();
  uint16_t getChannelValue(uint16_t channel);
  uint32_t getLastUpdateTime();
  DMXState getState();
}

// SerialInterface Interface
namespace SerialInterface {
  bool initialize();
  bool update();
  bool sendStatus();
  bool processIncomingCommands();
  bool sendResponse(const char* message);
}

// SystemConfig Interface
namespace SystemConfigMgr {
  bool initialize();
  bool loadFromEEPROM();
  bool saveToEEPROM();
  bool validateConfig();
  bool resetToDefaults();
  SystemConfig* getConfig();
}

// SafetyMonitor Interface
namespace SafetyMonitor {
  bool initialize();
  bool update();
  bool checkLimitSwitches();
  bool checkStepperAlarm();
  SafetyState getSafetyState();
  bool isEmergencyStopActive();
}

#endif // GLOBALINTERFACE_H