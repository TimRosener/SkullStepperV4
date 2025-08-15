// ============================================================================
// File: StepperController.h
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.1.13
// Date: 2025-02-08
// Author: Tim Rosener
// Description: Thread-safe stepper motor control with ODStepper integration
// License: MIT
//
// IMPORTANT: This module runs on Core 0 for real-time performance
// ============================================================================

#ifndef STEPPERCONTROLLER_H
#define STEPPERCONTROLLER_H

#include <Arduino.h>
#include "GlobalInterface.h"

// ============================================================================
// StepperController Namespace - Thread-Safe Motion Control
// ============================================================================

namespace StepperController {
    
    // ------------------------------------------------------------------------
    // Public Interface Functions
    // ------------------------------------------------------------------------
    
    /**
     * Initialize the stepper controller on Core 0
     * Creates real-time task, initializes ODStepper, sets up limit switches
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * Update function (called from Core 0 task)
     * Processes commands, monitors limits, updates status
     * @return true if update successful
     */
    bool update();
    
    /**
     * Process a motion command from the queue
     * Thread-safe wrapper for ODStepper operations
     * @param cmd Motion command to process
     * @return true if command accepted
     */
    bool processMotionCommand(const MotionCommand& cmd);
    
    /**
     * Emergency stop with maximum deceleration
     * Thread-safe, can be called from any core
     * @return true if stop initiated
     */
    bool emergencyStop();
    
    /**
     * Enable or disable the stepper motor
     * @param state true to enable, false to disable
     * @return true if state changed successfully
     */
    bool enable(bool state);
    
    /**
     * Get current position in steps (thread-safe)
     * @return current commanded position
     */
    int32_t getCurrentPosition();
    
    /**
     * Get current speed in steps/sec (thread-safe)
     * @return current speed (signed for direction)
     */
    float getCurrentSpeed();
    
    /**
     * Get current motion state (thread-safe)
     * @return current motion state enum
     */
    MotionState getMotionState();
    
    /**
     * Check if motor is currently moving
     * @return true if motor is in motion
     */
    bool isMoving();
    
    /**
     * Get current motion profile settings
     * @return copy of current motion profile
     */
    MotionProfile getMotionProfile();
    
    /**
     * Set motion profile parameters
     * @param profile New motion profile settings
     * @return true if parameters accepted
     */
    bool setMotionProfile(const MotionProfile& profile);
    
    /**
     * Check if stepper is enabled
     * @return true if enabled
     */
    bool isEnabled();
    
    /**
     * Get current step frequency for diagnostics
     * @return step frequency in Hz
     */
    float getStepFrequency();
    
    /**
     * Get timing diagnostics for debugging
     * @param stepInterval Returns current step interval in microseconds
     * @param dutyCycle Returns actual duty cycle percentage
     * @return true if diagnostics available
     */
    bool getTimingDiagnostics(uint32_t& stepInterval, float& dutyCycle);
    
    // ------------------------------------------------------------------------
    // Homing and Limit Functions
    // ------------------------------------------------------------------------
    
    /**
     * Start homing sequence with auto-range detection
     * Finds left limit, then right limit to determine range
     * @return true if homing started
     */
    bool startHoming();
    
    /**
     * Check if homing is in progress
     * @return true if currently homing
     */
    bool isHoming();
    
    /**
     * Get homing progress
     * @return 0-100 percentage complete
     */
    uint8_t getHomingProgress();
    
    /**
     * Check if system has been homed
     * @return true if homing completed successfully
     */
    bool isHomed();
    
    /**
     * Get detected position limits from homing
     * @param minPos Returns minimum position (left limit + margin)
     * @param maxPos Returns maximum position (right limit - margin)
     * @return true if limits are valid (homed)
     */
    bool getPositionLimits(int32_t& minPos, int32_t& maxPos);
    
    /**
     * Check limit switch states
     * @param leftLimit Returns left limit state
     * @param rightLimit Returns right limit state
     */
    void getLimitStates(bool& leftLimit, bool& rightLimit);
    
    // ------------------------------------------------------------------------
    // Advanced Motion Functions
    // ------------------------------------------------------------------------
    
    /**
     * Move to absolute position with limit checking
     * @param position Target position in steps
     * @return true if move accepted
     */
    bool moveTo(int32_t position);
    
    /**
     * Move relative to current position
     * @param steps Number of steps to move (signed)
     * @return true if move accepted
     */
    bool move(int32_t steps);
    
    /**
     * Stop with normal deceleration
     * @return true if stop initiated
     */
    bool stop();
    
    /**
     * Set maximum speed
     * @param speed Maximum speed in steps/sec
     * @return true if speed accepted
     */
    bool setMaxSpeed(float speed);
    
    /**
     * Set acceleration/deceleration
     * @param accel Acceleration in steps/sec²
     * @return true if acceleration accepted
     */
    bool setAcceleration(float accel);
    
    /**
     * Get distance to target position
     * @return steps remaining to target (signed)
     */
    int32_t distanceToGo();
    
    /**
     * Check if CL57Y ALARM is active
     * @return true if alarm signal detected
     */
    bool isAlarmActive();
    
    /**
     * Enable or disable step timing diagnostics
     * @param enable true to enable diagnostics
     * @return true if successful
     */
    bool enableStepDiagnostics(bool enable);
    
    /**
     * Check if a limit fault is active
     * @return true if limit fault requires homing to clear
     */
    bool isLimitFaultActive();
    
    /**
     * Check if the task is healthy (responding within timeout)
     * @return true if task has updated within last 5 seconds
     */
    bool isTaskHealthy();
    
    /**
     * Get the last task update timestamp
     * @return milliseconds since last task update
     */
    uint32_t getLastTaskUpdateTime();
    
} // namespace StepperController

#endif // STEPPERCONTROLLER_H