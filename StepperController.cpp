// ============================================================================
// File: StepperController.cpp
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.0.0
// Date: 2025-01-28
// Author: Tim Rosener
// Description: Thread-safe stepper motor control implementation with ODStepper
// License: MIT
//
// CRITICAL: This module runs on Core 0 for real-time performance
// ============================================================================

#include "StepperController.h"
#include "HardwareConfig.h"
#include <ODStepper.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// ============================================================================
// Static Variables and Constants
// ============================================================================

namespace StepperController {

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

// Motion profile
static MotionProfile g_currentProfile = {
    .maxSpeed = DEFAULT_MAX_SPEED,
    .acceleration = DEFAULT_ACCELERATION,
    .deceleration = DEFAULT_ACCELERATION,  // FastAccelStepper uses same value
    .jerk = 1000.0f,
    .targetPosition = 0,
    .enableLimits = true
};

// Limit switch handling
static volatile bool g_leftLimitTriggered = false;
static volatile bool g_rightLimitTriggered = false;
static bool g_leftLimitState = false;
static bool g_rightLimitState = false;
static uint32_t g_leftLimitDebounceTime = 0;
static uint32_t g_rightLimitDebounceTime = 0;

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
static const int32_t HOMING_SPEED = 500;  // steps/sec for homing (increased from 100)
static const int32_t BACKOFF_STEPS = 50;  // steps to back off from limits
static const int32_t POSITION_MARGIN = 10; // safety margin from limits
static const uint32_t HOMING_TIMEOUT_MS = 30000; // 30 second timeout for finding limits
static uint32_t g_homingStartTime = 0;
static uint32_t g_homingPhaseStartTime = 0;

// CL57Y ALARM monitoring
static bool g_alarmState = false;
static uint32_t g_lastAlarmCheck = 0;

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
 * Handle limit switch flags with debouncing
 * Called from Core 0 task only
 */
static void handleLimitFlags() {
    uint32_t currentTime = millis();
    
    // Check left limit with debouncing
    if (g_leftLimitTriggered) {
        bool currentState = (digitalRead(LEFT_LIMIT_PIN) == LOW);
        
        if (currentState != g_leftLimitState) {
            if (currentTime - g_leftLimitDebounceTime > LIMIT_SWITCH_DEBOUNCE) {
                g_leftLimitState = currentState;
                g_leftLimitDebounceTime = currentTime;
                
                if (g_leftLimitState) {
                    Serial.println("StepperController: Left limit activated");
                    
                    // Handle based on current state
                    if (g_homingState != HomingState::FINDING_LEFT) {
                        // Emergency stop if not homing
                        if (g_stepper && g_stepper->isRunning()) {
                            g_stepper->forceStop();
                            g_motionState = MotionState::IDLE;
                            SAFE_WRITE_STATUS(safetyState, SafetyState::LEFT_LIMIT_ACTIVE);
                        }
                    }
                }
            }
        }
    }
    
    // Check right limit with debouncing
    if (g_rightLimitTriggered) {
        bool currentState = (digitalRead(RIGHT_LIMIT_PIN) == LOW);
        
        if (currentState != g_rightLimitState) {
            if (currentTime - g_rightLimitDebounceTime > LIMIT_SWITCH_DEBOUNCE) {
                g_rightLimitState = currentState;
                g_rightLimitDebounceTime = currentTime;
                
                if (g_rightLimitState) {
                    Serial.println("StepperController: Right limit activated");
                    
                    // Handle based on current state
                    if (g_homingState != HomingState::FINDING_RIGHT) {
                        // Emergency stop if not homing
                        if (g_stepper && g_stepper->isRunning()) {
                            g_stepper->forceStop();
                            g_motionState = MotionState::IDLE;
                            SAFE_WRITE_STATUS(safetyState, SafetyState::RIGHT_LIMIT_ACTIVE);
                        }
                    }
                }
            }
        }
    }
    
    // Update status based on current states
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
                g_stepper->setSpeedInHz(HOMING_SPEED);
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
                // Finished backing off, set operating limits
                g_maxPosition = g_stepper->getCurrentPosition() - POSITION_MARGIN;
                g_positionLimitsValid = true;
                
                // Move to center of range
                int32_t centerPosition = (g_minPosition + g_maxPosition) / 2;
                g_stepper->setSpeedInHz(g_currentProfile.maxSpeed);
                g_stepper->moveTo(centerPosition);
                g_homingState = HomingState::MOVING_TO_CENTER;
                
                Serial.printf("StepperController: Operating range: %d to %d steps\n", 
                             g_minPosition, g_maxPosition);
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
                
                uint32_t homingTime = millis() - g_homingStartTime;
                Serial.println("StepperController: Homing complete!");
                Serial.printf("StepperController: Position range: %d to %d (%d total steps)\n",
                             g_minPosition, g_maxPosition, g_maxPosition - g_minPosition);
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
    
    // Set slow speed for homing
    g_stepper->setSpeedInHz(HOMING_SPEED);
    g_stepper->setAcceleration(g_currentProfile.acceleration);
    
    // Start moving toward left limit - use a large distance
    g_stepper->moveTo(-100000); // Move far enough to find limit
    g_motionState = MotionState::HOMING;
    
    Serial.printf("StepperController: Homing at %d steps/sec, timeout %lu ms\n", 
                  HOMING_SPEED, HOMING_TIMEOUT_MS);
}

// ============================================================================
// Core 0 Task Implementation
// ============================================================================

void stepperControllerTask(void* parameter) {
    const TickType_t xFrequency = pdMS_TO_TICKS(1); // 1ms for reasonable response time
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    Serial.println("StepperController: Core 0 task started");
    
    while (true) {
        // ====================================================================
        // Check limit switch flags from ISR (every cycle - 1ms)
        // ====================================================================
        if (g_leftLimitTriggered || g_rightLimitTriggered) {
            handleLimitFlags();
            g_leftLimitTriggered = false;
            g_rightLimitTriggered = false;
        }
        
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
        // Check CL57Y ALARM (every 10 cycles = 10ms)
        // ====================================================================
        static uint8_t alarmCounter = 0;
        if (++alarmCounter >= 10) {
            alarmCounter = 0;
            checkAlarmStatus();
        }
        
        // Precise 1ms timing
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
    g_stepper->setAutoEnable(true);
    
    // Set initial motion parameters
    g_stepper->setSpeedInHz(g_currentProfile.maxSpeed);
    g_stepper->setAcceleration(g_currentProfile.acceleration);
    g_stepper->setCurrentPosition(0);
    
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
    
    // Initialize limit states
    g_leftLimitState = (digitalRead(LEFT_LIMIT_PIN) == LOW);
    g_rightLimitState = (digitalRead(RIGHT_LIMIT_PIN) == LOW);
    
    g_initialized = true;
    
    Serial.println("StepperController: Initialization complete");
    Serial.println("StepperController: Running on Core 0 for real-time performance");
    Serial.printf("StepperController: Limit switches - Left: %s, Right: %s\n",
                  g_leftLimitState ? "ACTIVE" : "inactive",
                  g_rightLimitState ? "ACTIVE" : "inactive");
    
    return true;
}

bool processMotionCommand(const MotionCommand& cmd) {
    if (!g_initialized || g_stepper == nullptr) {
        return false;
    }
    
    if (xSemaphoreTake(g_stepperMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        Serial.println("StepperController: Failed to acquire mutex for command");
        return false;
    }
    
    bool success = false;
    
    switch (cmd.type) {
        case CommandType::MOVE_ABSOLUTE:
            if (g_positionLimitsValid && cmd.profile.enableLimits) {
                // Clamp to valid range
                int32_t targetPos = constrain(cmd.profile.targetPosition, 
                                            g_minPosition, g_maxPosition);
                g_stepper->moveTo(targetPos);
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
                    targetPos = constrain(targetPos, g_minPosition, g_maxPosition);
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
    cmd.timestamp = millis();
    
    return xQueueSend(g_motionCommandQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE;
}

bool move(int32_t steps) {
    MotionCommand cmd;
    cmd.type = CommandType::MOVE_RELATIVE;
    cmd.profile = g_currentProfile;
    cmd.profile.targetPosition = steps;
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

bool update() {
    // This function is called from Core 0 task
    // All updates happen in the task loop
    return g_initialized;
}

} // namespace StepperController