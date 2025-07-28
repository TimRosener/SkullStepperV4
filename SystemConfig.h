// ============================================================================
// File: SystemConfig.h
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.0.0
// Date: 2025-01-23
// Author: Tim Rosener
// Description: SystemConfig module interface - flash-based configuration management
// License: MIT
// Phase: 2 (Complete) - Configuration Management
// ============================================================================

#ifndef SYSTEMCONFIG_H
#define SYSTEMCONFIG_H

#include "GlobalInterface.h"
#include "HardwareConfig.h"

// ============================================================================
// SystemConfig Module - Core 1 Configuration Management
// ============================================================================

namespace SystemConfigMgr {
  
  // ----------------------------------------------------------------------------
  // Module Interface Functions (Phase 1: Stub implementations)
  // ----------------------------------------------------------------------------
  
  /**
   * Initialize the system configuration module
   * Sets up EEPROM access and loads configuration
   * @return true if initialization successful
   */
  bool initialize();
  
  /**
   * Load configuration from EEPROM
   * @return true if configuration loaded successfully
   */
  bool loadFromEEPROM();
  
  /**
   * Save current configuration to EEPROM
   * @return true if configuration saved successfully
   */
  bool saveToEEPROM();
  
  /**
   * Validate current configuration integrity
   * @return true if configuration is valid
   */
  bool validateConfig();
  
  /**
   * Reset configuration to factory defaults
   * @return true if reset successful
   */
  bool resetToDefaults();
  
  // getConfig() is declared in GlobalInterface.h - no need to redeclare here
  
  /**
   * Get read-only configuration pointer
   * @return const pointer to current config structure
   */
  const SystemConfig* getConfigReadOnly();
  
  /**
   * Commit configuration changes
   * Validates and applies pending changes
   * @return true if changes committed successfully
   */
  bool commitChanges();
  
  // ----------------------------------------------------------------------------
  // Parameter Access Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Set motion profile parameters
   * @param profile new motion profile
   * @return true if profile valid and set
   */
  bool setMotionProfile(const MotionProfile& profile);
  
  /**
   * Get current motion profile
   * @return current motion profile
   */
  MotionProfile getMotionProfile();
  
  /**
   * Set position limits
   * @param minPos minimum position (steps)
   * @param maxPos maximum position (steps)
   * @return true if limits valid and set
   */
  bool setPositionLimits(int32_t minPos, int32_t maxPos);
  
  /**
   * Set DMX configuration
   * @param startChannel DMX start channel (1-512)
   * @param scale scaling factor (DMX to steps)
   * @param offset position offset (steps)
   * @return true if DMX config valid and set
   */
  bool setDMXConfig(uint16_t startChannel, float scale, int32_t offset);
  
  /**
   * Set safety configuration
   * @param enableLimits enable limit switch monitoring
   * @param enableAlarm enable stepper alarm monitoring
   * @param emergencyDecel emergency deceleration rate
   * @return true if safety config valid and set
   */
  bool setSafetyConfig(bool enableLimits, bool enableAlarm, float emergencyDecel);
  
  // ----------------------------------------------------------------------------
  // Parameter Validation Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Validate motion profile parameters
   * @param profile profile to validate
   * @return true if profile is valid
   */
  bool validateMotionProfile(const MotionProfile& profile);
  
  /**
   * Validate position limits
   * @param minPos minimum position
   * @param maxPos maximum position
   * @return true if limits are valid
   */
  bool validatePositionLimits(int32_t minPos, int32_t maxPos);
  
  /**
   * Validate DMX configuration
   * @param startChannel start channel
   * @param scale scaling factor
   * @param offset position offset
   * @return true if DMX config is valid
   */
  bool validateDMXConfig(uint16_t startChannel, float scale, int32_t offset);
  
  // ----------------------------------------------------------------------------
  // Configuration Export/Import Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Export configuration as JSON string
   * @param buffer buffer to write JSON (must be large enough)
   * @param bufferSize size of provided buffer
   * @return number of characters written, 0 if error
   */
  size_t exportToJSON(char* buffer, size_t bufferSize);
  
  /**
   * Import configuration from JSON string
   * @param jsonString JSON configuration string
   * @return true if JSON parsed and configuration imported
   */
  bool importFromJSON(const char* jsonString);
  
  /**
   * Get configuration checksum
   * @return current configuration checksum
   */
  uint16_t getChecksum();
  
  /**
   * Get configuration version
   * @return current configuration version
   */
  uint32_t getVersion();
  
  // ----------------------------------------------------------------------------
  // EEPROM Management Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Check if EEPROM contains valid configuration
   * @return true if valid configuration found
   */
  bool isEEPROMValid();
  
  /**
   * Erase EEPROM configuration area
   * @return true if erase successful
   */
  bool eraseEEPROM();
  
  /**
   * Get EEPROM usage statistics
   * @param totalBytes returns total EEPROM size
   * @param usedBytes returns bytes used by configuration
   * @return true if statistics retrieved
   */
  bool getEEPROMStats(size_t& totalBytes, size_t& usedBytes);
}

#endif // SYSTEMCONFIG_H