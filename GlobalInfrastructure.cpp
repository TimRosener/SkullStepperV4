// ============================================================================
// File: GlobalInfrastructure.cpp
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.0.0
// Date: 2025-01-23
// Author: Tim Rosener
// Description: Thread-safe global infrastructure implementation
// License: MIT
// ============================================================================

#include "GlobalInterface.h"
#include "HardwareConfig.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_timer.h>

// ============================================================================
// Global Variable Definitions (Thread-Safe)
// ============================================================================

// System status and configuration (protected by mutexes)
SystemStatus g_systemStatus = {};
SystemConfig g_systemConfig = {};

// FreeRTOS synchronization primitives
SemaphoreHandle_t g_statusMutex = nullptr;
SemaphoreHandle_t g_configMutex = nullptr;

// Inter-module communication queues
QueueHandle_t g_motionCommandQueue = nullptr;
QueueHandle_t g_statusUpdateQueue = nullptr;
QueueHandle_t g_dmxDataQueue = nullptr;

// System state management
static SystemState g_currentSystemState = SystemState::UNINITIALIZED;
static SemaphoreHandle_t g_systemStateMutex = nullptr;
static uint32_t g_systemStartTime = 0;

// Thread safety initialization flag
static bool g_globalInfrastructureInitialized = false;

// ============================================================================
// Thread-Safe Global Infrastructure Functions
// ============================================================================

/**
 * Initialize all global infrastructure with thread safety
 * Must be called before any other system modules
 * @return true if initialization successful
 */
bool initializeGlobalInfrastructure() {
    if (g_globalInfrastructureInitialized) {
        Serial.println("GlobalInfrastructure: Already initialized");
        return true;
    }
    
    Serial.println("GlobalInfrastructure: Initializing thread-safe infrastructure...");
    
    // Record system start time
    g_systemStartTime = millis();
    
    // ========================================================================
    // Create FreeRTOS Mutexes for Thread Safety
    // ========================================================================
    
    g_statusMutex = xSemaphoreCreateMutex();
    if (g_statusMutex == nullptr) {
        Serial.println("GlobalInfrastructure: FATAL - Failed to create status mutex");
        return false;
    }
    
    g_configMutex = xSemaphoreCreateMutex();
    if (g_configMutex == nullptr) {
        Serial.println("GlobalInfrastructure: FATAL - Failed to create config mutex");
        vSemaphoreDelete(g_statusMutex);
        return false;
    }
    
    g_systemStateMutex = xSemaphoreCreateMutex();
    if (g_systemStateMutex == nullptr) {
        Serial.println("GlobalInfrastructure: FATAL - Failed to create system state mutex");
        vSemaphoreDelete(g_statusMutex);
        vSemaphoreDelete(g_configMutex);
        return false;
    }
    
    Serial.println("GlobalInfrastructure: Thread-safe mutexes created");
    
    // ========================================================================
    // Create FreeRTOS Queues for Inter-Module Communication
    // ========================================================================
    
    // Motion command queue (SerialInterface -> StepperController)
    g_motionCommandQueue = xQueueCreate(10, sizeof(MotionCommand));
    if (g_motionCommandQueue == nullptr) {
        Serial.println("GlobalInfrastructure: FATAL - Failed to create motion command queue");
        // Clean up mutexes
        vSemaphoreDelete(g_statusMutex);
        vSemaphoreDelete(g_configMutex);
        vSemaphoreDelete(g_systemStateMutex);
        return false;
    }
    
    // Status update queue (all modules -> monitoring)
    g_statusUpdateQueue = xQueueCreate(20, sizeof(SystemStatus));
    if (g_statusUpdateQueue == nullptr) {
        Serial.println("GlobalInfrastructure: FATAL - Failed to create status update queue");
        // Clean up
        vQueueDelete(g_motionCommandQueue);
        vSemaphoreDelete(g_statusMutex);
        vSemaphoreDelete(g_configMutex);
        vSemaphoreDelete(g_systemStateMutex);
        return false;
    }
    
    // DMX data queue (DMXReceiver -> StepperController)
    g_dmxDataQueue = xQueueCreate(5, sizeof(uint16_t));
    if (g_dmxDataQueue == nullptr) {
        Serial.println("GlobalInfrastructure: FATAL - Failed to create DMX data queue");
        // Clean up
        vQueueDelete(g_motionCommandQueue);
        vQueueDelete(g_statusUpdateQueue);
        vSemaphoreDelete(g_statusMutex);
        vSemaphoreDelete(g_configMutex);
        vSemaphoreDelete(g_systemStateMutex);
        return false;
    }
    
    Serial.println("GlobalInfrastructure: Inter-module communication queues created");
    Serial.printf("  Motion Command Queue: %d slots\n", 10);
    Serial.printf("  Status Update Queue: %d slots\n", 20);
    Serial.printf("  DMX Data Queue: %d slots\n", 5);
    
    // ========================================================================
    // Initialize Global Data Structures with Thread-Safe Defaults
    // ========================================================================
    
    // Initialize system status with safe defaults
    if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        g_systemStatus.systemState = SystemState::INITIALIZING;
        g_systemStatus.motionState = MotionState::IDLE;
        g_systemStatus.safetyState = SafetyState::NORMAL;
        g_systemStatus.dmxState = DMXState::NO_SIGNAL;
        
        g_systemStatus.currentPosition = 0;
        g_systemStatus.targetPosition = 0;
        g_systemStatus.currentSpeed = 0.0f;
        
        g_systemStatus.stepperEnabled = false;
        g_systemStatus.limitsActive[0] = false; // Left limit
        g_systemStatus.limitsActive[1] = false; // Right limit
        g_systemStatus.stepperAlarm = false;
        
        g_systemStatus.dmxChannel = DMX_START_CHANNEL;
        g_systemStatus.dmxValue = 0;
        g_systemStatus.lastDMXUpdate = 0;
        
        g_systemStatus.uptime = 0;
        g_systemStatus.errorCode = 0;
        
        xSemaphoreGive(g_statusMutex);
        Serial.println("GlobalInfrastructure: System status initialized with thread-safe defaults");
    } else {
        Serial.println("GlobalInfrastructure: ERROR - Failed to acquire status mutex for initialization");
        return false;
    }
    
    // Initialize system configuration with safe defaults
    if (xSemaphoreTake(g_configMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Motion profile defaults
        g_systemConfig.defaultProfile.maxSpeed = DEFAULT_MAX_SPEED;
        g_systemConfig.defaultProfile.acceleration = DEFAULT_ACCELERATION;
        g_systemConfig.defaultProfile.deceleration = DEFAULT_ACCELERATION;
        g_systemConfig.defaultProfile.jerk = 1000.0f;
        g_systemConfig.defaultProfile.targetPosition = 0;
        g_systemConfig.defaultProfile.enableLimits = true;
        
        // Position limits
        g_systemConfig.homePositionPercent = 50.0f;
        g_systemConfig.minPosition = MIN_POSITION_STEPS;
        g_systemConfig.maxPosition = MAX_POSITION_STEPS;
        
        // DMX settings
        g_systemConfig.dmxStartChannel = DMX_START_CHANNEL;
        g_systemConfig.dmxScale = 1.0f;
        g_systemConfig.dmxOffset = 0;
        g_systemConfig.dmxTimeout = 5000;
        
        // Safety settings
        g_systemConfig.enableLimitSwitches = true;
        g_systemConfig.enableStepperAlarm = true;
        g_systemConfig.emergencyDeceleration = EMERGENCY_STOP_DECEL;
        
        // System settings
        g_systemConfig.statusUpdateInterval = STATUS_UPDATE_INTERVAL_MS;
        g_systemConfig.enableSerialOutput = true;
        g_systemConfig.serialVerbosity = 2;
        
        // Validation
        g_systemConfig.configVersion = 0x00040000; // Version 4.0.0
        g_systemConfig.checksum = 0; // Will be calculated later
        
        xSemaphoreGive(g_configMutex);
        Serial.println("GlobalInfrastructure: System configuration initialized with thread-safe defaults");
    } else {
        Serial.println("GlobalInfrastructure: ERROR - Failed to acquire config mutex for initialization");
        return false;
    }
    
    // Set initial system state
    setSystemState(SystemState::READY);
    
    g_globalInfrastructureInitialized = true;
    
    Serial.println("GlobalInfrastructure: *** THREAD-SAFE INITIALIZATION COMPLETE ***");
    Serial.println("GlobalInfrastructure: All mutexes, queues, and data structures ready");
    Serial.println("GlobalInfrastructure: System is memory-safe and thread-safe");
    
    return true;
}

/**
 * Clean shutdown of global infrastructure
 * Safely deletes all FreeRTOS objects
 */
void shutdownGlobalInfrastructure() {
    if (!g_globalInfrastructureInitialized) {
        return;
    }
    
    Serial.println("GlobalInfrastructure: Shutting down thread-safe infrastructure...");
    
    // Delete queues
    if (g_motionCommandQueue) {
        vQueueDelete(g_motionCommandQueue);
        g_motionCommandQueue = nullptr;
    }
    
    if (g_statusUpdateQueue) {
        vQueueDelete(g_statusUpdateQueue);
        g_statusUpdateQueue = nullptr;
    }
    
    if (g_dmxDataQueue) {
        vQueueDelete(g_dmxDataQueue);
        g_dmxDataQueue = nullptr;
    }
    
    // Delete mutexes
    if (g_statusMutex) {
        vSemaphoreDelete(g_statusMutex);
        g_statusMutex = nullptr;
    }
    
    if (g_configMutex) {
        vSemaphoreDelete(g_configMutex);
        g_configMutex = nullptr;
    }
    
    if (g_systemStateMutex) {
        vSemaphoreDelete(g_systemStateMutex);
        g_systemStateMutex = nullptr;
    }
    
    g_globalInfrastructureInitialized = false;
    
    Serial.println("GlobalInfrastructure: Shutdown complete");
}

// ============================================================================
// Thread-Safe Utility Functions Implementation
// ============================================================================

/**
 * Get system uptime in milliseconds (thread-safe)
 * @return uptime since system start in milliseconds
 */
uint32_t getSystemUptime() {
    return millis() - g_systemStartTime;
}

/**
 * Set system state with thread safety
 * @param newState new system state to set
 */
void setSystemState(SystemState newState) {
    if (!g_globalInfrastructureInitialized || g_systemStateMutex == nullptr) {
        // Fallback for early initialization
        g_currentSystemState = newState;
        return;
    }
    
    if (xSemaphoreTake(g_systemStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        SystemState oldState = g_currentSystemState;
        g_currentSystemState = newState;
        
        // Also update the global system status
        SAFE_WRITE_STATUS(systemState, newState);
        
        xSemaphoreGive(g_systemStateMutex);
        
        // Log state changes for debugging
        if (oldState != newState) {
            Serial.printf("GlobalInfrastructure: System state changed: %d -> %d\n", 
                         (int)oldState, (int)newState);
        }
    } else {
        Serial.println("GlobalInfrastructure: WARNING - Failed to acquire system state mutex");
    }
}

/**
 * Get current system state with thread safety
 * @return current system state
 */
SystemState getSystemState() {
    if (!g_globalInfrastructureInitialized || g_systemStateMutex == nullptr) {
        return g_currentSystemState;
    }
    
    SystemState currentState;
    if (xSemaphoreTake(g_systemStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        currentState = g_currentSystemState;
        xSemaphoreGive(g_systemStateMutex);
    } else {
        Serial.println("GlobalInfrastructure: WARNING - Failed to acquire system state mutex for read");
        currentState = g_currentSystemState; // Return last known state
    }
    
    return currentState;
}

/**
 * Calculate 16-bit checksum for data integrity (thread-safe)
 * @param data pointer to data to checksum
 * @param length length of data in bytes
 * @return 16-bit checksum
 */
uint16_t calculateChecksum(const void* data, size_t length) {
    if (!data || length == 0) {
        return 0;
    }
    
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t sum = 0;
    
    // Simple additive checksum with overflow handling
    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    
    // Fold 32-bit sum to 16-bit with carry
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return static_cast<uint16_t>(~sum); // One's complement
}

// ============================================================================
// Thread-Safe Status Update Functions
// ============================================================================

/**
 * Update system uptime in status structure (thread-safe)
 */
void updateSystemUptime() {
    uint32_t uptime = getSystemUptime();
    SAFE_WRITE_STATUS(uptime, uptime);
}

/**
 * Broadcast system status update to monitoring queue (thread-safe)
 * @return true if status update queued successfully
 */
bool broadcastStatusUpdate() {
    if (!g_globalInfrastructureInitialized || g_statusUpdateQueue == nullptr) {
        return false;
    }
    
    SystemStatus statusCopy;
    
    // Make a thread-safe copy of current status
    if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        statusCopy = g_systemStatus;
        xSemaphoreGive(g_statusMutex);
    } else {
        return false;
    }
    
    // Update uptime before broadcasting
    statusCopy.uptime = getSystemUptime();
    
    // Try to queue the status update (non-blocking)
    if (xQueueSend(g_statusUpdateQueue, &statusCopy, 0) == pdTRUE) {
        return true;
    } else {
        // Queue full - not necessarily an error for status updates
        return false;
    }
}

// ============================================================================
// Memory Safety Validation Functions
// ============================================================================

/**
 * Validate system infrastructure integrity (thread-safe)
 * @return true if all infrastructure is healthy
 */
bool validateSystemIntegrity() {
    if (!g_globalInfrastructureInitialized) {
        Serial.println("GlobalInfrastructure: FAIL - Infrastructure not initialized");
        return false;
    }
    
    // Check mutexes
    if (g_statusMutex == nullptr || g_configMutex == nullptr || g_systemStateMutex == nullptr) {
        Serial.println("GlobalInfrastructure: FAIL - One or more mutexes are null");
        return false;
    }
    
    // Check queues
    if (g_motionCommandQueue == nullptr || g_statusUpdateQueue == nullptr || g_dmxDataQueue == nullptr) {
        Serial.println("GlobalInfrastructure: FAIL - One or more queues are null");
        return false;
    }
    
    // Test mutex acquisition (with timeout)
    if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        xSemaphoreGive(g_statusMutex);
    } else {
        Serial.println("GlobalInfrastructure: FAIL - Status mutex appears to be deadlocked");
        return false;
    }
    
    if (xSemaphoreTake(g_configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        xSemaphoreGive(g_configMutex);
    } else {
        Serial.println("GlobalInfrastructure: FAIL - Config mutex appears to be deadlocked");
        return false;
    }
    
    // Check queue space availability
    UBaseType_t motionQueueSpaces = uxQueueSpacesAvailable(g_motionCommandQueue);
    UBaseType_t statusQueueSpaces = uxQueueSpacesAvailable(g_statusUpdateQueue);
    UBaseType_t dmxQueueSpaces = uxQueueSpacesAvailable(g_dmxDataQueue);
    
    Serial.printf("GlobalInfrastructure: Queue status - Motion: %d/%d, Status: %d/%d, DMX: %d/%d\n",
                  (10 - motionQueueSpaces), 10,
                  (20 - statusQueueSpaces), 20, 
                  (5 - dmxQueueSpaces), 5);
    
    Serial.println("GlobalInfrastructure: System integrity validation PASSED");
    return true;
}

/**
 * Get memory usage statistics (thread-safe)
 * @param freeHeap returns free heap memory in bytes
 * @param minFreeHeap returns minimum free heap since boot
 * @return true if statistics retrieved successfully
 */
bool getMemoryStats(uint32_t& freeHeap, uint32_t& minFreeHeap) {
    freeHeap = esp_get_free_heap_size();
    minFreeHeap = esp_get_minimum_free_heap_size();
    return true;
}

// ============================================================================
// Infrastructure Status and Debug Functions
// ============================================================================

/**
 * Print complete infrastructure status for debugging
 */
void printInfrastructureStatus() {
    Serial.println("\n=== Global Infrastructure Status ===");
    
    Serial.printf("Initialization: %s\n", g_globalInfrastructureInitialized ? "COMPLETE" : "INCOMPLETE");
    Serial.printf("System State: %d\n", (int)getSystemState());
    Serial.printf("Uptime: %u ms\n", getSystemUptime());
    
    // Memory statistics
    uint32_t freeHeap, minFreeHeap;
    if (getMemoryStats(freeHeap, minFreeHeap)) {
        Serial.printf("Free Heap: %u bytes\n", freeHeap);
        Serial.printf("Min Free Heap: %u bytes\n", minFreeHeap);
    }
    
    // FreeRTOS object status
    if (g_globalInfrastructureInitialized) {
        Serial.println("FreeRTOS Objects:");
        Serial.printf("  Status Mutex: %s\n", g_statusMutex ? "OK" : "NULL");
        Serial.printf("  Config Mutex: %s\n", g_configMutex ? "OK" : "NULL");
        Serial.printf("  System State Mutex: %s\n", g_systemStateMutex ? "OK" : "NULL");
        Serial.printf("  Motion Command Queue: %s\n", g_motionCommandQueue ? "OK" : "NULL");
        Serial.printf("  Status Update Queue: %s\n", g_statusUpdateQueue ? "OK" : "NULL");
        Serial.printf("  DMX Data Queue: %s\n", g_dmxDataQueue ? "OK" : "NULL");
        
        // Queue usage
        if (g_motionCommandQueue) {
            UBaseType_t motionUsed = 10 - uxQueueSpacesAvailable(g_motionCommandQueue);
            Serial.printf("  Motion Queue Usage: %d/10\n", motionUsed);
        }
        
        if (g_statusUpdateQueue) {
            UBaseType_t statusUsed = 20 - uxQueueSpacesAvailable(g_statusUpdateQueue);
            Serial.printf("  Status Queue Usage: %d/20\n", statusUsed);
        }
        
        if (g_dmxDataQueue) {
            UBaseType_t dmxUsed = 5 - uxQueueSpacesAvailable(g_dmxDataQueue);
            Serial.printf("  DMX Queue Usage: %d/5\n", dmxUsed);
        }
    }
    
    Serial.println("=====================================\n");
}