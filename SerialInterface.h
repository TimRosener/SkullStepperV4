// ============================================================================
// File: SerialInterface.h
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.0.0
// Date: 2025-01-23
// Author: Tim Rosener
// Description: SerialInterface module interface - command processing and communication
// License: MIT
// Phase: 3 (Complete) - Serial Communication Interface
// ============================================================================

#ifndef SERIALINTERFACE_H
#define SERIALINTERFACE_H

#include "GlobalInterface.h"
#include "HardwareConfig.h"
#include <ArduinoJson.h>

// ============================================================================
// SerialInterface Module - Core 1 Serial Communication
// ============================================================================

namespace SerialInterface {
  
  // ----------------------------------------------------------------------------
  // Module Interface Functions (Phase 1: Stub implementations)
  // ----------------------------------------------------------------------------
  
  /**
   * Initialize the serial interface module
   * Sets up command processing and response formatting
   * @return true if initialization successful
   */
  bool initialize();
  
  /**
   * Update function called every 10ms from Core 1
   * Processes incoming serial commands and sends status updates
   * @return true if update successful
   */
  bool update();
  
  /**
   * Send current system status via serial
   * @return true if status sent successfully
   */
  bool sendStatus();
  
  /**
   * Process incoming serial commands
   * Handles both human-readable and JSON commands
   * @return true if commands processed successfully
   */
  bool processIncomingCommands();
  
  /**
   * Send response message via serial
   * @param message response message to send
   * @return true if message sent successfully
   */
  bool sendResponse(const char* message);
  
  // ----------------------------------------------------------------------------
  // Command Processing Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Process human-readable command
   * @param command command string (e.g., "MOVE 1000", "HOME", "STATUS")
   * @return true if command processed successfully
   */
  bool processHumanCommand(const char* command);
  
  /**
   * Process JSON command
   * @param jsonCommand JSON command string
   * @return true if JSON command processed successfully
   */
  bool processJSONCommand(const char* jsonCommand);
  
  /**
   * Set command echo mode
   * @param enable true to echo commands, false to disable
   * @return true if echo mode set
   */
  bool setEchoMode(bool enable);
  
  /**
   * Set verbose output mode
   * @param level verbosity level (0=minimal, 1=normal, 2=verbose, 3=debug)
   * @return true if verbosity level set
   */
  bool setVerbosity(uint8_t level);
  
  // ----------------------------------------------------------------------------
  // Status Reporting Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Send system status in human-readable format
   * @return true if status sent
   */
  bool sendHumanStatus();
  
  /**
   * Send system status in JSON format
   * @return true if JSON status sent
   */
  bool sendJSONStatus();
  
  /**
   * Send configuration in JSON format
   * @return true if JSON config sent
   */
  bool sendJSONConfig();
  
  /**
   * Send help information
   * @return true if help sent
   */
  bool sendHelp();
  
  // ----------------------------------------------------------------------------
  // Motion Command Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Send motion command to stepper controller
   * @param cmd motion command to send
   * @return true if command queued successfully
   */
  bool sendMotionCommand(const MotionCommand& cmd);
  
  /**
   * Create motion command from parameters
   * @param type command type
   * @param target target position (for position commands)
   * @param speed speed override (0 = use default)
   * @return motion command structure
   */
  MotionCommand createMotionCommand(CommandType type, int32_t target = 0, float speed = 0);
  
  // ----------------------------------------------------------------------------
  // Additional Implementation Functions (Phase 3)
  // ----------------------------------------------------------------------------
  
  /**
   * Process configuration set command (human interface)
   * @param parameter parameter name to set
   * @param value parameter value as string
   * @return true if parameter set successfully
   */
  bool processConfigSet(const char* parameter, const char* value);
  
  /**
   * Process JSON configuration set command
   * @param setObj JSON object containing parameters to set
   * @return true if configuration updated successfully
   */
  bool processJSONConfigSet(ArduinoJson::JsonObject setObj);
  
  /**
   * Process configuration reset command (reset individual parameter)
   * @param parameter parameter name to reset to default
   * @return true if parameter reset successfully
   */
  bool processConfigReset(const char* parameter);
  
  /**
   * Process factory reset command (reset all configuration)
   * @return true if factory reset completed successfully
   */
  bool processFactoryReset();
}

#endif // SERIALINTERFACE_H