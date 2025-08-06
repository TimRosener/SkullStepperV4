// ============================================================================
// File: StepperController.cpp
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.1.1
// Date: 2025-02-02
// Author: Tim Rosener
// Description: Thread-safe stepper motor control implementation with ODStepper
//              Fixed auto-home on E-stop to properly use processMotionCommand()
// License: MIT
//
// CRITICAL: This module runs on Core 0 for real-time performance
// ============================================================================

#include "StepperController.h"
#include "HardwareConfig.h"
#include "SystemConfig.h"
#include <ODStepper.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// ============================================================================
// Static Variables and Constants
// ============================================================================

namespace StepperController {

// Helper function to convert motion state to string
static const char* motionStateToString(MotionState state) {
    switch (state) {
        case MotionState::IDLE: return "IDLE";
        case MotionState::ACCELERATING: return "ACCELERATING";
        case MotionState::CONSTANT_VELOCITY: return "CONSTANT_VELOCITY";
        case MotionState::DECELERATING: return "DECELERATING";
        case MotionState::HOMING: return "HOMING";
        default: return "UNKNOWN";
    }
}

// Forward declarations of helper functions
static void handleLimitFlags();
static void updateHomingSequence();
static void updateMotionStatus();
static void checkAlarmStatus();
static void startHomingSequence();

// Task and synchronization
static TaskHandle_t g_stepperTaskHandle = nullptr;
static SemaphoreHandle_t g_stepperMutex = nullptr;
static bool g_initialized = false;

// ODStepper/FastAccelStepper instances
static ODStepperEngine g_engine = ODStepperEngine();
static ODStepper* g_stepper = nullptr;

// Position tracking and limits
static int32_t g_currentPosition = 0;
static int32_t g_minPosition = MIN_POSITION_STEPS;
static int32_t g_maxPosition = MAX_POSITION_STEPS;
static bool g_positionLimitsValid = false;
static bool g_systemHomed = false;

// Motion state
static MotionState g_motionState = MotionState::IDLE;
static bool g_stepperEnabled = false;
static float g_currentSpeed = 0.0f;
static bool g_limitFaultActive = false;  // Latched limit fault flag

// Diagnostic timing data
static bool g_enableStepDiagnostics = false;  // Diagnostics disabled - mechanical issue resolved
static uint32_t g_lastStepTime = 0;
static uint32_t g_stepIntervals[10];  // Store last 10 step intervals
static uint8_t g_stepIntervalIndex = 0;
static MotionState g_lastMotionState = MotionState::IDLE;

// Motion profile
static MotionProfile g_currentProfile = {
    .maxSpeed = DEFAULT_MAX_SPEED,
    .acceleration = DEFAULT_ACCELERATION,
    .deceleration = DEFAULT_ACCELERATION,  // FastAccelStepper uses same value
    .jerk = 1000.0f,
    .targetPosition = 0,
    .enableLimits = true
};

// Limit switch handling with noise filtering
static volatile bool g_leftLimitTriggered = false;
static volatile bool g_rightLimitTriggered = false;
static bool g_leftLimitState = false;
static bool g_rightLimitState = false;
static uint32_t g_leftLimitDebounceStart = 0;  // When debounce period started
static uint32_t g_rightLimitDebounceStart = 0;

// Cache for continuous monitoring
static bool g_lastLeftPinReading = false;
static bool g_lastRightPinReading = false;

// Homing state machine
enum class HomingState {
    IDLE,
    FINDING_LEFT,
    BACKING_OFF_LEFT,
    FINDING_RIGHT,
    BACKING_OFF_RIGHT,
    MOVING_TO_CENTER,
    COMPLETE,
    ERROR
};

static HomingState g_homingState = HomingState::IDLE;
static uint8_t g_homingProgress = 0;
static int32_t g_detectedRightLimit = 0;
static float g_homingSpeed = 940.0f;  // Homing speed (steps/sec) - loaded from config
static const int32_t BACKOFF_STEPS = 50;  // steps to back off from limits
static const int32_t POSITION_MARGIN = 10; // safety margin from limits
static const uint32_t HOMING_TIMEOUT_MS = 90000; // 90 second timeout for finding limits (3x longer for full travel)
static uint32_t g_homingStartTime = 0;
static uint32_t g_homingPhaseStartTime = 0;

// CL57Y ALARM monitoring
static bool g_alarmState = false;
static uint32_t g_lastAlarmCheck = 0;

// Auto-home after E-stop
static bool g_autoHomeRequested = false;
static uint32_t g_autoHomeRequestTime = 0;
static const uint32_t AUTO_HOME_DELAY_MS = 2000;  // 2 second delay after E-stop

// ============================================================================
// Interrupt Service Routines (MINIMAL!)
// ============================================================================

void IRAM_ATTR leftLimitISR() {
    g_leftLimitTriggered = true;  // Single instruction only!
}

void IRAM_ATTR rightLimitISR() {
    g_rightLimitTriggered = true;  // Single instruction only!
}

// ============================================================================
// Internal Helper Functions (Definitions must come before use)
// ============================================================================

/**
 * Process left limit switch state change
 * Called from Core 0 task only
 */
static void processLeftLimit(bool currentReading, uint32_t currentTime) {
    // Check if state is different from our known state
    if (currentReading != g_leftLimitState) {
        // Start or continue debounce timer
        if (g_leftLimitDebounceStart == 0) {
            g_leftLimitDebounceStart = currentTime;
        } else if (currentTime - g_leftLimitDebounceStart >= LIMIT_SWITCH_DEBOUNCE) {
            // State has been stable for debounce period - confirm the change
            g_leftLimitState = currentReading;
            g_leftLimitDebounceStart = 0;
            g_leftLimitTriggered = false;  // Clear interrupt flag after processing
            
            if (g_leftLimitState) {
            Serial.println("StepperController: Left limit ACTIVATED");
            
            // Handle based on current state
            if (g_homingState != HomingState::FINDING_LEFT) {
            // Emergency stop if not homing
            if (g_stepper && g_stepper->isRunning()) {
            // Use emergency stop for proper state handling
            g_stepper->forceStop();
            g_motionState = MotionState::IDLE;
            SAFE_WRITE_STATUS(safetyState, SafetyState::EMERGENCY_STOP);
            g_limitFaultActive = true;  // Latch the fault
            Serial.println("StepperController: EMERGENCY STOP - Left limit hit!");
            Serial.println("StepperController: FAULT LATCHED - Homing required to clear.");
                
                    // Check if auto-home on E-stop is enabled
                    SystemConfig* config = SystemConfigMgr::getConfig();
                    if (config && config->autoHomeOnEstop) {
                    Serial.println("StepperController: AUTO-HOME ON E-STOP enabled - Will start homing after delay...");
                    g_autoHomeRequested = true;
                        g_autoHomeRequestTime = millis();
                        }
                    }
                }
            } else {
                Serial.println("StepperController: Left limit released");
                // Note: Fault remains latched until cleared by successful homing
            }
        }
    } else {
        // State matches - reset debounce timer and clear flag
        g_leftLimitDebounceStart = 0;
        g_leftLimitTriggered = false;
    }
}

/**
 * Process right limit switch state change
 * Called from Core 0 task only
 */
static void processRightLimit(bool currentReading, uint32_t currentTime) {
    // Check if state is different from our known state
    if (currentReading != g_rightLimitState) {
        // Start or continue debounce timer
        if (g_rightLimitDebounceStart == 0) {
            g_rightLimitDebounceStart = currentTime;
        } else if (currentTime - g_rightLimitDebounceStart >= LIMIT_SWITCH_DEBOUNCE) {
            // State has been stable for debounce period - confirm the change
            g_rightLimitState = currentReading;
            g_rightLimitDebounceStart = 0;
            g_rightLimitTriggered = false;  // Clear interrupt flag after processing
            
            if (g_rightLimitState) {
            Serial.println("StepperController: Right limit ACTIVATED");
            
            // Handle based on current state
            if (g_homingState != HomingState::FINDING_RIGHT) {
            // Emergency stop if not homing
            if (g_stepper && g_stepper->isRunning()) {
            // Use emergency stop for proper state handling
            g_stepper->forceStop();
            g_motionState = MotionState::IDLE;
            SAFE_WRITE_STATUS(safetyState, SafetyState::EMERGENCY_STOP);
            g_limitFaultActive = true;  // Latch the fault
            Serial.println("StepperController: EMERGENCY STOP - Right limit hit!");
            Serial.println("StepperController: FAULT LATCHED - Homing required to clear.");
                
                    // Check if auto-home on E-stop is enabled
                        SystemConfig* config = SystemConfigMgr::getConfig();
                        if (config && config->autoHomeOnEstop) {
                            Serial.println("StepperController: AUTO-HOME ON E-STOP enabled - Will start homing after delay...");
                            g_autoHomeRequested = true;
                            g_autoHomeRequestTime = millis();
                        }
                    }
                }
            } else {
                Serial.println("StepperController: Right limit released");
                // Note: Fault remains latched until cleared by successful homing
            }
        }
    } else {
        // State matches - reset debounce timer and clear flag
        g_rightLimitDebounceStart = 0;
        g_rightLimitTriggered = false;
    }
}

/**
 * Check limit switches with continuous monitoring
 * Called from Core 0 task only
 */
static void checkLimitSwitches() {
    uint32_t currentTime = millis();
    
    // Always read current pin states (continuous monitoring)
    bool leftPinReading = (digitalRead(LEFT_LIMIT_PIN) == LOW);
    bool rightPinReading = (digitalRead(RIGHT_LIMIT_PIN) == LOW);
    
    // Process left limit if pin changed OR interrupt fired
    if (leftPinReading != g_lastLeftPinReading || g_leftLimitTriggered) {
        processLeftLimit(leftPinReading, currentTime);
        g_lastLeftPinReading = leftPinReading;
    }
    
    // Process right limit if pin changed OR interrupt fired
    if (rightPinReading != g_lastRightPinReading || g_rightLimitTriggered) {
        processRightLimit(rightPinReading, currentTime);
        g_lastRightPinReading = rightPinReading;
    }
    
    // Update global status
    SAFE_WRITE_STATUS(limitsActive[0], g_leftLimitState);
    SAFE_WRITE_STATUS(limitsActive[1], g_rightLimitState);
}

/**
 * Update homing sequence state machine
 * Called from Core 0 task only
 */
static void updateHomingSequence() {
    if (xSemaphoreTake(g_stepperMutex, pdMS_TO_TICKS(1)) != pdTRUE) {
        return; // Skip this cycle if can't get mutex quickly
    }
    
    // Check for overall homing timeout
    if (millis() - g_homingStartTime > HOMING_TIMEOUT_MS) {
        g_homingState = HomingState::ERROR;
        g_stepper->forceStop();
        Serial.println("StepperController: ERROR - Homing timeout!");
    }
    
    switch (g_homingState) {
        case HomingState::FINDING_LEFT:
            g_homingProgress = 10;
            if (g_leftLimitState) {
                // Found left limit
                g_stepper->forceStop();
                g_homingState = HomingState::BACKING_OFF_LEFT;
                g_homingPhaseStartTime = millis();
                Serial.println("StepperController: Found left limit");
            } else if (!g_stepper->isRunning()) {
                // Movement stopped without finding limit - error
                g_homingState = HomingState::ERROR;
                Serial.println("StepperController: ERROR - Left limit not found");
            }
            break;
            
        case HomingState::BACKING_OFF_LEFT:
            g_homingProgress = 25;
            if (!g_stepper->isRunning()) {
                // Back off from limit
                g_stepper->move(BACKOFF_STEPS);
            } else if (g_stepper->isRunning() && !g_leftLimitState) {
                // Finished backing off, set home position
                g_stepper->setCurrentPosition(0);
                g_currentPosition = 0;
                g_minPosition = POSITION_MARGIN;
                
                // Start moving to find right limit - use a much larger distance
                g_stepper->setSpeedInHz(g_homingSpeed);
                g_stepper->moveTo(100000); // Move 100k steps - should hit limit before this
                g_homingState = HomingState::FINDING_RIGHT;
                g_homingPhaseStartTime = millis();
                Serial.println("StepperController: Home position set, finding right limit");
            }
            break;
            
        case HomingState::FINDING_RIGHT:
            g_homingProgress = 50;
            if (g_rightLimitState) {
                // Found right limit, record position
                g_detectedRightLimit = g_stepper->getCurrentPosition();
                g_stepper->forceStop();
                g_homingState = HomingState::BACKING_OFF_RIGHT;
                g_homingPhaseStartTime = millis();
                Serial.printf("StepperController: Found right limit at position %d\n", g_detectedRightLimit);
            } else if (!g_stepper->isRunning()) {
                // Movement stopped without finding limit - error
                g_homingState = HomingState::ERROR;
                Serial.println("StepperController: ERROR - Right limit not found (reached max travel)");
            } else if (millis() - g_homingPhaseStartTime > HOMING_TIMEOUT_MS) {
                // Timeout waiting for right limit
                g_stepper->forceStop();
                g_homingState = HomingState::ERROR;
                Serial.println("StepperController: ERROR - Right limit not found (timeout)");
            }
            break;
            
        case HomingState::BACKING_OFF_RIGHT:
            g_homingProgress = 75;
            if (!g_stepper->isRunning()) {
                // Back off from limit
                g_stepper->move(-BACKOFF_STEPS);
            } else if (g_stepper->isRunning() && !g_rightLimitState) {
                // Finished backing off, set physical operating limits
                g_maxPosition = g_stepper->getCurrentPosition() - POSITION_MARGIN;
                g_positionLimitsValid = true;
                
                // Get configuration
                SystemConfig* config = SystemConfigMgr::getConfig();
                if (config) {
                    // If user limits are not set or invalid, set them to physical limits
                    if (config->minPosition < g_minPosition || config->minPosition >= g_maxPosition) {
                        config->minPosition = g_minPosition;
                        Serial.printf("StepperController: Setting user minPosition to physical limit: %d\n", g_minPosition);
                    }
                    if (config->maxPosition > g_maxPosition || config->maxPosition <= g_minPosition) {
                        config->maxPosition = g_maxPosition;
                        Serial.printf("StepperController: Setting user maxPosition to physical limit: %d\n", g_maxPosition);
                    }
                    
                    // Save the updated configuration
                    SystemConfigMgr::saveToEEPROM();
                    
                    // Calculate home position based on configured percentage
                    float homePercent = config->homePositionPercent;
                    int32_t range = g_maxPosition - g_minPosition;
                    int32_t homePosition = g_minPosition + (int32_t)((range * homePercent) / 100.0f);
                    
                    // Move to configured home position
                    g_stepper->setSpeedInHz(g_currentProfile.maxSpeed);
                    g_stepper->moveTo(homePosition);
                    g_homingState = HomingState::MOVING_TO_CENTER;
                    
                    Serial.printf("StepperController: Physical range: %d to %d steps\n", 
                                 g_minPosition, g_maxPosition);
                    Serial.printf("StepperController: User-configured range: %d to %d steps\n",
                                 config->minPosition, config->maxPosition);
                    Serial.printf("StepperController: Moving to home position: %d (%.1f%% of range)\n",
                                 homePosition, homePercent);
                } else {
                    // No config, just use center position
                    int32_t homePosition = (g_minPosition + g_maxPosition) / 2;
                    g_stepper->setSpeedInHz(g_currentProfile.maxSpeed);
                    g_stepper->moveTo(homePosition);
                    g_homingState = HomingState::MOVING_TO_CENTER;
                    
                    Serial.printf("StepperController: No config available, moving to center: %d\n", homePosition);
                }
            }
            break;
            
        case HomingState::MOVING_TO_CENTER:
            g_homingProgress = 90;
            if (!g_stepper->isRunning()) {
                // Homing complete!
                g_homingState = HomingState::COMPLETE;
                g_homingProgress = 100;
                g_systemHomed = true;
                g_motionState = MotionState::IDLE;
                g_limitFaultActive = false;  // Clear any limit faults after successful homing
                SAFE_WRITE_STATUS(safetyState, SafetyState::NORMAL);  // Clear safety state
                
                uint32_t homingTime = millis() - g_homingStartTime;
                Serial.println("StepperController: Homing complete!");
                Serial.println("StepperController: Limit faults cleared.");
                Serial.printf("StepperController: Position range: %d to %d (%d total steps)\n",
                             g_minPosition, g_maxPosition, g_maxPosition - g_minPosition);
                Serial.printf("StepperController: Final position: %d\n", g_stepper->getCurrentPosition());
                Serial.printf("StepperController: Homing took %lu ms\n", homingTime);
            }
            break;
            
        case HomingState::ERROR:
            g_homingProgress = 0;
            g_motionState = MotionState::IDLE;
            SAFE_WRITE_STATUS(safetyState, SafetyState::POSITION_ERROR);
            break;
            
        default:
            break;
    }
    
    xSemaphoreGive(g_stepperMutex);
}

/**
 * Update motion status in global status structure
 * Called from Core 0 task only
 */
static void updateMotionStatus() {
    if (xSemaphoreTake(g_stepperMutex, pdMS_TO_TICKS(1)) != pdTRUE) {
        return; // Skip this cycle if can't get mutex quickly
    }
    
    // Update position and speed
    if (g_stepper) {
        g_currentPosition = g_stepper->getCurrentPosition();
        // Convert from milliHz to steps/sec
        int32_t speedMilliHz = g_stepper->getCurrentSpeedInMilliHz();
        g_currentSpeed = speedMilliHz / 1000.0f;
        
        // Update motion state
        if (g_homingState != HomingState::IDLE && 
            g_homingState != HomingState::COMPLETE &&
            g_homingState != HomingState::ERROR) {
            g_motionState = MotionState::HOMING;
        } else if (g_stepper->isRunning()) {
            // Determine motion phase based on speed and acceleration
            if (g_stepper->isRampGeneratorActive()) {
                uint8_t rampState = g_stepper->rampState();
                if (rampState & RAMP_STATE_ACCELERATING_FLAG) {
                    g_motionState = MotionState::ACCELERATING;
                } else if (rampState & RAMP_STATE_DECELERATING_FLAG) {
                    g_motionState = MotionState::DECELERATING;
                } else {
                    g_motionState = MotionState::CONSTANT_VELOCITY;
                }
            } else {
                g_motionState = MotionState::CONSTANT_VELOCITY;
            }
        } else {
            g_motionState = MotionState::IDLE;
        }
    }
    
    xSemaphoreGive(g_stepperMutex);
    
    // Update global status (thread-safe)
    SAFE_WRITE_STATUS(currentPosition, g_currentPosition);
    SAFE_WRITE_STATUS(currentSpeed, g_currentSpeed);
    SAFE_WRITE_STATUS(motionState, g_motionState);
    SAFE_WRITE_STATUS(stepperEnabled, g_stepperEnabled);
}

/**
 * Check CL57Y ALARM status
 * Called from Core 0 task only
 */
static void checkAlarmStatus() {
    bool alarmActive = (digitalRead(STEPPER_ALARM_PIN) == LOW); // Active low
    
    if (alarmActive != g_alarmState) {
        g_alarmState = alarmActive;
        SAFE_WRITE_STATUS(stepperAlarm, g_alarmState);
        
        if (g_alarmState) {
            Serial.println("StepperController: WARNING - CL57Y ALARM active!");
            SAFE_WRITE_STATUS(safetyState, SafetyState::STEPPER_ALARM);
            
            // Could trigger emergency stop here if desired
            // emergencyStop();
        }
    }
}

/**
 * Start homing sequence
 * Internal helper called with mutex already held
 */
static void startHomingSequence() {
    Serial.println("StepperController: Starting homing sequence...");
    
    // Reset homing state
    g_homingState = HomingState::FINDING_LEFT;
    g_homingProgress = 0;
    g_systemHomed = false;
    g_positionLimitsValid = false;
    g_homingStartTime = millis();
    g_homingPhaseStartTime = millis();
    
    // Reload homing speed from configuration in case it was changed
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (config) {
        g_homingSpeed = config->homingSpeed;
        Serial.printf("StepperController: Using homing speed: %.1f steps/sec\n", g_homingSpeed);
    }
    
    // Set homing speed from configuration
    g_stepper->setSpeedInHz(g_homingSpeed);
    g_stepper->setAcceleration(g_currentProfile.acceleration);
    
    // Check initial limit switch states
    if (g_leftLimitState && g_rightLimitState) {
        // Both limits active - error condition
        Serial.println("StepperController: ERROR - Both limit switches active!");
        g_homingState = HomingState::ERROR;
        g_motionState = MotionState::IDLE;
        return;
    }
    
    // Check if we're already at left limit
    if (g_leftLimitState) {
        Serial.println("StepperController: Already at left limit, backing off");
        g_homingState = HomingState::BACKING_OFF_LEFT;
        g_stepper->move(BACKOFF_STEPS * 2); // Back off a bit more since we don't know exact position
    } 
    // Check if we're at right limit (need to move left first)
    else if (g_rightLimitState) {
        Serial.println("StepperController: At right limit, moving to find left limit");
        g_stepper->moveTo(-100000); // Move left to find left limit
    }
    // Normal case - not at any limit
    else {
        Serial.println("StepperController: Not at limits, moving to find left limit");
        g_stepper->moveTo(-100000); // Move left to find left limit
    }
    
    g_motionState = MotionState::HOMING;
    
    Serial.printf("StepperController: Homing at %.1f steps/sec, timeout %lu ms\n", 
                  g_homingSpeed, HOMING_TIMEOUT_MS);
}

// ============================================================================
// Core 0 Task Implementation
// ============================================================================

void stepperControllerTask(void* parameter) {
    const TickType_t xFrequency = pdMS_TO_TICKS(2); // 2ms for less frequent GPIO reads
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    Serial.println("StepperController: Core 0 task started");
    Serial.println("StepperController: Continuous limit monitoring enabled (2ms interval)");
    
    while (true) {
        // ====================================================================
        // Check limit switches with continuous monitoring (every cycle - 2ms)
        // ====================================================================
        checkLimitSwitches();
        
        // ====================================================================
        // Process motion commands from queue (every cycle)
        // ====================================================================
        MotionCommand cmd;
        if (xQueueReceive(g_motionCommandQueue, &cmd, 0) == pdTRUE) {
            processMotionCommand(cmd);
        }
        
        // ====================================================================
        // Update homing sequence if in progress (every cycle)
        // ====================================================================
        if (g_homingState != HomingState::IDLE && 
            g_homingState != HomingState::COMPLETE &&
            g_homingState != HomingState::ERROR) {
            updateHomingSequence();
        }
        
        // ====================================================================
        // Update motion status (every cycle)
        // ====================================================================
        updateMotionStatus();
        
        // ====================================================================
        // Check CL57Y ALARM (every 10 cycles = 20ms)
        // ====================================================================
        static uint8_t alarmCounter = 0;
        if (++alarmCounter >= 10) {
            alarmCounter = 0;
            checkAlarmStatus();
        }
        
        // ====================================================================
        // Handle Auto-Home Request After E-Stop
        // ====================================================================
        if (g_autoHomeRequested) {
            // Debug output to track auto-home state
            static uint32_t lastDebugTime = 0;
            if (millis() - lastDebugTime > 1000) {  // Print debug every second
                Serial.printf("StepperController: Auto-home debug - requested=%d, running=%d, homingState=%d, timeElapsed=%lu, limitFault=%d\n",
                             g_autoHomeRequested, g_stepper->isRunning(), (int)g_homingState, 
                             millis() - g_autoHomeRequestTime, g_limitFaultActive);
                lastDebugTime = millis();
            }
            
            if (!g_stepper->isRunning() && 
                (g_homingState == HomingState::IDLE || g_homingState == HomingState::COMPLETE) &&
                (millis() - g_autoHomeRequestTime >= AUTO_HOME_DELAY_MS)) {
                
                Serial.println("StepperController: Starting automatic homing after E-stop...");
                
                // Clear the limit fault first to allow homing
                if (g_limitFaultActive) {
                    Serial.println("StepperController: Clearing limit fault to allow auto-homing...");
                    g_limitFaultActive = false;
                }
                
                g_autoHomeRequested = false;
                
                // Create a HOME command and process it directly
                MotionCommand homeCmd;
                homeCmd.type = CommandType::HOME;
                homeCmd.timestamp = millis();
                processMotionCommand(homeCmd);
            }
        }
        
        // Precise 2ms timing
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ============================================================================
// Public Interface Implementation
// ============================================================================

bool initialize() {
    if (g_initialized) {
        Serial.println("StepperController: Already initialized");
        return true;
    }
    
    Serial.println("StepperController: Initializing...");
    
    // Create mutex for thread-safe access
    g_stepperMutex = xSemaphoreCreateMutex();
    if (g_stepperMutex == nullptr) {
        Serial.println("StepperController: ERROR - Failed to create mutex");
        return false;
    }
    
    // Configure GPIO pins
    pinMode(LEFT_LIMIT_PIN, INPUT_PULLUP);
    pinMode(RIGHT_LIMIT_PIN, INPUT_PULLUP);
    pinMode(STEPPER_ALARM_PIN, INPUT_PULLUP);
    
    // Initialize ODStepper engine
    g_engine.init();
    
    // Create stepper connected to step pin
    g_stepper = g_engine.stepperConnectToPin(STEPPER_STEP_PIN);
    if (g_stepper == nullptr) {
        Serial.println("StepperController: ERROR - Failed to connect to step pin");
        vSemaphoreDelete(g_stepperMutex);
        return false;
    }
    
    // Configure stepper pins
    g_stepper->setDirectionPin(STEPPER_DIR_PIN);
    // Set enable pin with HIGH = enabled (inverted from default)
    g_stepper->setEnablePin(STEPPER_ENABLE_PIN, false);  // false = HIGH enables stepper
    g_stepper->setAutoEnable(false);  // Keep motor enabled to avoid startup issues
    
    // Load saved configuration from SystemConfig
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (config) {
        // Update motion profile with saved values
        g_currentProfile.maxSpeed = config->defaultProfile.maxSpeed;
        g_currentProfile.acceleration = config->defaultProfile.acceleration;
        g_currentProfile.deceleration = config->defaultProfile.acceleration; // Same value
        g_currentProfile.jerk = config->defaultProfile.jerk;
        g_currentProfile.enableLimits = config->defaultProfile.enableLimits;
        
        // Load homing speed
        g_homingSpeed = config->homingSpeed;
        
        Serial.printf("StepperController: Loaded saved config - Speed: %.1f, Accel: %.1f, Homing: %.1f\n",
                     g_currentProfile.maxSpeed, g_currentProfile.acceleration, g_homingSpeed);
    } else {
        Serial.println("StepperController: WARNING - Using default config values");
    }
    
    // Set motion parameters
    g_stepper->setSpeedInHz(g_currentProfile.maxSpeed);
    g_stepper->setAcceleration(g_currentProfile.acceleration);
    g_stepper->setCurrentPosition(0);
    
    // Enable the stepper
    g_stepper->enableOutputs();
    g_stepperEnabled = true;
    
    // Attach limit switch interrupts (minimal ISRs)
    attachInterrupt(digitalPinToInterrupt(LEFT_LIMIT_PIN), leftLimitISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RIGHT_LIMIT_PIN), rightLimitISR, CHANGE);
    
    // Create Core 0 task for real-time control
    BaseType_t result = xTaskCreatePinnedToCore(
        stepperControllerTask,      // Task function
        "StepperCtrl",             // Name
        4096,                      // Stack size
        nullptr,                   // Parameters
        2,                         // Priority (higher than normal)
        &g_stepperTaskHandle,      // Task handle
        0                          // Core 0 - CRITICAL!
    );
    
    if (result != pdPASS) {
        Serial.println("StepperController: ERROR - Failed to create Core 0 task");
        g_stepper = nullptr;
        vSemaphoreDelete(g_stepperMutex);
        return false;
    }
    
    // Initialize limit states and cache
    g_leftLimitState = (digitalRead(LEFT_LIMIT_PIN) == LOW);
    g_rightLimitState = (digitalRead(RIGHT_LIMIT_PIN) == LOW);
    g_lastLeftPinReading = g_leftLimitState;
    g_lastRightPinReading = g_rightLimitState;
    
    // Clear interrupt flags at startup
    g_leftLimitTriggered = false;
    g_rightLimitTriggered = false;
    
    g_initialized = true;
    
    Serial.println("StepperController: Initialization complete");
    Serial.println("StepperController: Running on Core 0 for real-time performance");
    Serial.printf("StepperController: Limit switches - Left: %s, Right: %s\n",
                  g_leftLimitState ? "ACTIVE" : "inactive",
                  g_rightLimitState ? "ACTIVE" : "inactive");
    
    // Clear message about homing requirement
    Serial.println("\n*** IMPORTANT: HOMING REQUIRED ***");
    Serial.println("The system must be homed before any movement is allowed.");
    Serial.println("Use the HOME command to establish position limits.");
    Serial.println("Movement commands will be rejected until homing is complete.\n");
    
    return true;
}

bool processMotionCommand(const MotionCommand& cmd) {
    if (!g_initialized || g_stepper == nullptr) {
        return false;
    }
    
    if (xSemaphoreTake(g_stepperMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        Serial.println("StepperController: Failed to acquire mutex for command");
        // Release any held resources and return
        return false;
    }
    
    bool success = false;
    
    // Check if homing is required before allowing motion
    bool isMotionCommand = (cmd.type == CommandType::MOVE_ABSOLUTE || 
                           cmd.type == CommandType::MOVE_RELATIVE);
    
    if (isMotionCommand) {
        // Check for limit fault
        if (g_limitFaultActive) {
            Serial.println("StepperController: REJECTED - Limit fault active. Home required.");
            SAFE_WRITE_STATUS(safetyState, SafetyState::POSITION_ERROR);
            xSemaphoreGive(g_stepperMutex);
            return false;
        }
        
        // Check if system has been homed
        if (!g_systemHomed) {
            Serial.println("StepperController: REJECTED - System not homed. Home required before movement.");
            SAFE_WRITE_STATUS(safetyState, SafetyState::POSITION_ERROR);
            xSemaphoreGive(g_stepperMutex);
            return false;
        }
        
        // Check if position limits are valid
        if (!g_positionLimitsValid) {
            Serial.println("StepperController: REJECTED - Position limits not established. Home required.");
            SAFE_WRITE_STATUS(safetyState, SafetyState::POSITION_ERROR);
            xSemaphoreGive(g_stepperMutex);
            return false;
        }
    }
    
    // Also reject speed/acceleration changes if there's a limit fault
    if (g_limitFaultActive && 
        (cmd.type == CommandType::SET_SPEED ||
         cmd.type == CommandType::SET_ACCELERATION)) {
        Serial.println("StepperController: REJECTED - Limit fault active. Home required.");
        SAFE_WRITE_STATUS(safetyState, SafetyState::POSITION_ERROR);
        xSemaphoreGive(g_stepperMutex);
        return false;
    }
    
    switch (cmd.type) {
        case CommandType::MOVE_ABSOLUTE:
            if (g_positionLimitsValid && cmd.profile.enableLimits) {
                // Get user-configured limits from SystemConfig
                SystemConfig* config = SystemConfigMgr::getConfig();
                if (config) {
                    // Use user-configured limits, not physical limits
                    int32_t userMinPos = config->minPosition;
                    int32_t userMaxPos = config->maxPosition;
                    
                    // Ensure user limits are within physical limits
                    userMinPos = constrain(userMinPos, g_minPosition, g_maxPosition);
                    userMaxPos = constrain(userMaxPos, g_minPosition, g_maxPosition);
                    
                    // Clamp target to user-configured range
                    int32_t targetPos = constrain(cmd.profile.targetPosition, 
                                                userMinPos, userMaxPos);
                    g_stepper->moveTo(targetPos);
                    
                    if (targetPos != cmd.profile.targetPosition) {
                        Serial.printf("StepperController: Move clamped from %d to %d (user limits: %d-%d)\n", 
                                    cmd.profile.targetPosition, targetPos, userMinPos, userMaxPos);
                    }
                } else {
                    // Fallback to physical limits if config not available
                    int32_t targetPos = constrain(cmd.profile.targetPosition, 
                                                g_minPosition, g_maxPosition);
                    g_stepper->moveTo(targetPos);
                }
            } else {
                // No limits or limits disabled
                g_stepper->moveTo(cmd.profile.targetPosition);
            }
            success = true;
            Serial.printf("StepperController: Move to %d\n", cmd.profile.targetPosition);
            break;
            
        case CommandType::MOVE_RELATIVE:
            {
                int32_t targetPos = g_currentPosition + cmd.profile.targetPosition;
                if (g_positionLimitsValid && cmd.profile.enableLimits) {
                    // Get user-configured limits from SystemConfig
                    SystemConfig* config = SystemConfigMgr::getConfig();
                    if (config) {
                        // Use user-configured limits, not physical limits
                        int32_t userMinPos = config->minPosition;
                        int32_t userMaxPos = config->maxPosition;
                        
                        // Ensure user limits are within physical limits
                        userMinPos = constrain(userMinPos, g_minPosition, g_maxPosition);
                        userMaxPos = constrain(userMaxPos, g_minPosition, g_maxPosition);
                        
                        // Clamp target to user-configured range
                        targetPos = constrain(targetPos, userMinPos, userMaxPos);
                        
                        if (targetPos != (g_currentPosition + cmd.profile.targetPosition)) {
                            Serial.printf("StepperController: Relative move clamped to %d (user limits: %d-%d)\n", 
                                        targetPos, userMinPos, userMaxPos);
                        }
                    } else {
                        // Fallback to physical limits if config not available
                        targetPos = constrain(targetPos, g_minPosition, g_maxPosition);
                    }
                }
                g_stepper->moveTo(targetPos);
                success = true;
                Serial.printf("StepperController: Move relative %d\n", cmd.profile.targetPosition);
            }
            break;
            
        case CommandType::SET_SPEED:
            g_stepper->setSpeedInHz(cmd.profile.maxSpeed);
            g_currentProfile.maxSpeed = cmd.profile.maxSpeed;
            success = true;
            Serial.printf("StepperController: Set speed to %.1f\n", cmd.profile.maxSpeed);
            break;
            
        case CommandType::SET_ACCELERATION:
            g_stepper->setAcceleration(cmd.profile.acceleration);
            g_currentProfile.acceleration = cmd.profile.acceleration;
            g_currentProfile.deceleration = cmd.profile.acceleration; // Same value
            success = true;
            Serial.printf("StepperController: Set acceleration to %.1f\n", cmd.profile.acceleration);
            break;
            
        case CommandType::HOME:
            if (g_homingState == HomingState::IDLE || 
                g_homingState == HomingState::COMPLETE ||
                g_homingState == HomingState::ERROR) {
                startHomingSequence();
                success = true;
            }
            break;
            
        case CommandType::STOP:
            g_stepper->stopMove();
            success = true;
            Serial.println("StepperController: Stop commanded");
            break;
            
        case CommandType::EMERGENCY_STOP:
            g_stepper->forceStop();
            g_motionState = MotionState::IDLE;
            SAFE_WRITE_STATUS(safetyState, SafetyState::EMERGENCY_STOP);
            success = true;
            Serial.println("StepperController: EMERGENCY STOP!");
            break;
            
        case CommandType::ENABLE:
            g_stepper->enableOutputs();
            g_stepperEnabled = true;
            success = true;
            Serial.println("StepperController: Outputs enabled");
            break;
            
        case CommandType::DISABLE:
            g_stepper->disableOutputs();
            g_stepperEnabled = false;
            success = true;
            Serial.println("StepperController: Outputs disabled");
            break;
            
        default:
            Serial.printf("StepperController: Unknown command type %d\n", (int)cmd.type);
            break;
    }
    
    xSemaphoreGive(g_stepperMutex);
    return success;
}

// Thread-safe public interface functions

bool emergencyStop() {
    MotionCommand cmd;
    cmd.type = CommandType::EMERGENCY_STOP;
    cmd.timestamp = millis();
    
    // Try to queue command for Core 0 processing
    if (xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE) {
        return true;
    }
    
    // If queue fails, try direct access (emergency!)
    if (g_initialized && g_stepper) {
        if (xSemaphoreTake(g_stepperMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            g_stepper->forceStop();
            xSemaphoreGive(g_stepperMutex);
            return true;
        }
    }
    
    return false;
}

bool enable(bool state) {
    MotionCommand cmd;
    cmd.type = state ? CommandType::ENABLE : CommandType::DISABLE;
    cmd.timestamp = millis();
    
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

int32_t getCurrentPosition() {
    int32_t position = 0;
    SAFE_READ_STATUS(currentPosition, position);
    return position;
}

float getCurrentSpeed() {
    float speed = 0.0f;
    SAFE_READ_STATUS(currentSpeed, speed);
    return speed;
}

MotionState getMotionState() {
    MotionState state = MotionState::IDLE;
    SAFE_READ_STATUS(motionState, state);
    return state;
}

bool isMoving() {
    return getMotionState() != MotionState::IDLE;
}

MotionProfile getMotionProfile() {
    MotionProfile profile;
    if (xSemaphoreTake(g_stepperMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        profile = g_currentProfile;
        xSemaphoreGive(g_stepperMutex);
    }
    return profile;
}

bool setMotionProfile(const MotionProfile& profile) {
    MotionCommand cmd;
    cmd.type = CommandType::SET_SPEED;
    cmd.profile = profile;
    cmd.timestamp = millis();
    
    // Queue speed change
    if (xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
        return false;
    }
    
    // Queue acceleration change
    cmd.type = CommandType::SET_ACCELERATION;
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

bool isEnabled() {
    bool enabled = false;
    SAFE_READ_STATUS(stepperEnabled, enabled);
    return enabled;
}

float getStepFrequency() {
    return abs(getCurrentSpeed()); // Steps/sec = frequency
}

bool getTimingDiagnostics(uint32_t& stepInterval, float& dutyCycle) {
    float speed = getCurrentSpeed();
    if (speed == 0) {
        stepInterval = 0;
        dutyCycle = 0;
        return true;
    }
    
    stepInterval = (uint32_t)(1000000.0f / abs(speed)); // microseconds
    dutyCycle = 25.0f; // ODStepper/FastAccelStepper uses ~25% duty cycle
    return true;
}

bool startHoming() {
    if (!g_initialized) return false;
    
    MotionCommand cmd;
    cmd.type = CommandType::HOME;
    cmd.timestamp = millis();
    
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

bool isHoming() {
    return (g_homingState != HomingState::IDLE && 
            g_homingState != HomingState::COMPLETE &&
            g_homingState != HomingState::ERROR);
}

uint8_t getHomingProgress() {
    return g_homingProgress;
}

bool isHomed() {
    return g_systemHomed;
}

bool getPositionLimits(int32_t& minPos, int32_t& maxPos) {
    if (!g_positionLimitsValid) {
        return false;
    }
    
    minPos = g_minPosition;
    maxPos = g_maxPosition;
    return true;
}

void getLimitStates(bool& leftLimit, bool& rightLimit) {
    leftLimit = g_leftLimitState;
    rightLimit = g_rightLimitState;
}

bool moveTo(int32_t position) {
    MotionCommand cmd;
    cmd.type = CommandType::MOVE_ABSOLUTE;
    cmd.profile = g_currentProfile;
    cmd.profile.targetPosition = position;
    cmd.profile.enableLimits = true;  // Ensure limits are enforced
    cmd.timestamp = millis();
    
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

bool move(int32_t steps) {
    MotionCommand cmd;
    cmd.type = CommandType::MOVE_RELATIVE;
    cmd.profile = g_currentProfile;
    cmd.profile.targetPosition = steps;
    cmd.profile.enableLimits = true;  // Ensure limits are enforced
    cmd.timestamp = millis();
    
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

bool stop() {
    MotionCommand cmd;
    cmd.type = CommandType::STOP;
    cmd.timestamp = millis();
    
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

bool setMaxSpeed(float speed) {
    if (speed <= 0 || speed > MAX_STEP_FREQUENCY) {
        return false;
    }
    
    MotionCommand cmd;
    cmd.type = CommandType::SET_SPEED;
    cmd.profile = g_currentProfile;
    cmd.profile.maxSpeed = speed;
    cmd.timestamp = millis();
    
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

bool setAcceleration(float accel) {
    if (accel <= 0) {
        return false;
    }
    
    MotionCommand cmd;
    cmd.type = CommandType::SET_ACCELERATION;
    cmd.profile = g_currentProfile;
    cmd.profile.acceleration = accel;
    cmd.profile.deceleration = accel; // FastAccelStepper uses same value
    cmd.timestamp = millis();
    
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

int32_t distanceToGo() {
    if (!g_initialized || g_stepper == nullptr) {
        return 0;
    }
    
    int32_t distance = 0;
    if (xSemaphoreTake(g_stepperMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // Calculate distance from current position to target
        distance = g_stepper->targetPos() - g_stepper->getCurrentPosition();
        xSemaphoreGive(g_stepperMutex);
    }
    
    return distance;
}

bool isAlarmActive() {
    bool alarm = false;
    SAFE_READ_STATUS(stepperAlarm, alarm);
    return alarm;
}

bool enableStepDiagnostics(bool enable) {
    g_enableStepDiagnostics = enable;
    if (enable) {
        Serial.println("StepperController: Step timing diagnostics ENABLED");
        Serial.println("  - Will report motion state transitions");
        Serial.println("  - Will capture step intervals at transitions");
        Serial.println("  - Will flag unusual step timings");
        // Clear diagnostic data
        memset(g_stepIntervals, 0, sizeof(g_stepIntervals));
        g_stepIntervalIndex = 0;
        g_lastStepTime = 0;
    } else {
        Serial.println("StepperController: Step timing diagnostics DISABLED");
    }
    return true;
}

bool isLimitFaultActive() {
    return g_limitFaultActive;
}

bool update() {
    // This function is called from Core 0 task
    // All updates happen in the task loop
    return g_initialized;
}

} // namespace StepperController