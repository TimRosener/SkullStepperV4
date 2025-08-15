// ============================================================================
// File: SafetyMonitor.h
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.1.13
// Date: 2025-02-08
// Author: Tim Rosener
// Description: SafetyMonitor module interface - limit switches and safety systems
// License: MIT
// Phase: 1 (Stub Implementation) - Ready for Phase 5 development
// ============================================================================

#ifndef SAFETYMONITOR_H
#define SAFETYMONITOR_H

#include "GlobalInterface.h"
#include "HardwareConfig.h"

// ============================================================================
// SafetyMonitor Module - Core 0 Real-Time Safety Monitoring
// ============================================================================

namespace SafetyMonitor {
  
  // ----------------------------------------------------------------------------
  // Module Interface Functions (Phase 1: Stub implementations)
  // ----------------------------------------------------------------------------
  
  /**
   * Initialize the safety monitor module
   * Sets up limit switch monitoring and safety systems
   * @return true if initialization successful
   */
  bool initialize();
  
  /**
   * Update function called every millisecond from Core 0
   * Monitors limit switches, stepper alarm, and safety conditions
   * @return true if update successful
   */
  bool update();
  
  /**
   * Check limit switch states with debouncing
   * @return true if limit switches checked successfully
   */
  bool checkLimitSwitches();
  
  /**
   * Check stepper driver alarm signal
   * @return true if stepper alarm checked successfully
   */
  bool checkStepperAlarm();
  
  /**
   * Get current safety state
   * @return current safety state enum
   */
  SafetyState getSafetyState();
  
  /**
   * Check if emergency stop is active
   * @return true if emergency stop condition exists
   */
  bool isEmergencyStopActive();
  
  // ----------------------------------------------------------------------------
  // Limit Switch Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Get left limit switch state
   * @return true if left limit switch is active
   */
  bool isLeftLimitActive();
  
  /**
   * Get right limit switch state
   * @return true if right limit switch is active
   */
  bool isRightLimitActive();
  
  /**
   * Set limit switch enable state
   * @param enable true to enable limit switch monitoring
   * @return true if state set successfully
   */
  bool enableLimitSwitches(bool enable);
  
  /**
   * Set limit switch debounce time
   * @param debounceMs debounce time in milliseconds
   * @return true if debounce time set
   */
  bool setDebounceTime(uint32_t debounceMs);
  
  // ----------------------------------------------------------------------------
  // Stepper Alarm Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Get stepper alarm state
   * @return true if stepper alarm is active
   */
  bool isStepperAlarmActive();
  
  /**
   * Enable stepper alarm monitoring
   * @param enable true to enable alarm monitoring
   * @return true if state set successfully
   */
  bool enableStepperAlarm(bool enable);
  
  /**
   * Clear stepper alarm condition
   * @return true if alarm cleared
   */
  bool clearStepperAlarm();
  
  // ----------------------------------------------------------------------------
  // Safety Action Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Trigger emergency stop sequence
   * @return true if emergency stop triggered
   */
  bool triggerEmergencyStop();
  
  /**
   * Clear safety fault conditions
   * @return true if faults cleared
   */
  bool clearSafetyFaults();
  
  /**
   * Check if position is within software limits
   * @param position position to check (steps)
   * @return true if position is safe
   */
  bool isPositionSafe(int32_t position);
  
  /**
   * Get safety fault history
   * @param buffer buffer to store fault codes
   * @param bufferSize size of buffer
   * @return number of faults retrieved
   */
  uint8_t getFaultHistory(uint16_t* buffer, uint8_t bufferSize);
}

#endif // SAFETYMONITOR_H