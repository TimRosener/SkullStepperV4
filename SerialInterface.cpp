// ============================================================================
// File: SerialInterface.cpp
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.0.0
// Date: 2025-01-23
// Author: Tim Rosener
// Description: SerialInterface module implementation - human and JSON command processing
// License: MIT
// Phase: 3 (Complete) - Serial Communication Interface
// ============================================================================

#include "SerialInterface.h"
#include "SystemConfig.h"
#include <ArduinoJson.h>

// ============================================================================
// SerialInterface Module - Phase 3 Full Implementation
// ============================================================================

namespace SerialInterface {
  
  // ----------------------------------------------------------------------------
  // Module State Variables
  // ----------------------------------------------------------------------------
  static bool g_initialized = false;
  static bool g_echoMode = true;
  static uint8_t g_verbosityLevel = 2;
  static bool g_statusStreaming = false;  // Default OFF - no automatic status streaming
  static bool g_jsonMode = false;         // Default OFF - human-readable output
  static uint32_t g_lastStatusTime = 0;
  static uint16_t g_commandCounter = 0;
  
  // Command buffer for processing
  static char g_commandBuffer[256];
  static size_t g_bufferIndex = 0;
  
  // Response buffer for formatting
  static char g_responseBuffer[512];
  
  // ----------------------------------------------------------------------------
  // Private Helper Functions
  // ----------------------------------------------------------------------------
  
  void printPrompt() {
    if (g_echoMode) {
      Serial.print("skull> ");
    }
  }
  
  void sendOK() {
    if (g_jsonMode) {
      Serial.println("{\"status\":\"ok\"}");
    } else {
      Serial.println("OK");
    }
  }
  
  void sendError(const char* message) {
    if (g_jsonMode) {
      Serial.printf("{\"status\":\"error\",\"message\":\"%s\"}\n", message);
    } else {
      Serial.print("ERROR: ");
      Serial.println(message);
    }
  }
  
  void sendInfo(const char* message) {
    if (g_verbosityLevel >= 1) {
      if (g_jsonMode) {
        Serial.printf("{\"status\":\"info\",\"message\":\"%s\"}\n", message);
      } else {
        Serial.print("INFO: ");
        Serial.println(message);
      }
    }
  }
  
  void sendDebug(const char* message) {
    if (g_verbosityLevel >= 3) {
      Serial.print("DEBUG: ");
      Serial.println(message);
    }
  }
  
  // Parse integer from command string
  bool parseInteger(const char* str, int32_t& value) {
    char* endPtr;
    value = strtol(str, &endPtr, 10);
    return (*endPtr == '\0' || *endPtr == ' ' || *endPtr == '\t');
  }
  
  // Parse float from command string
  bool parseFloat(const char* str, float& value) {
    char* endPtr;
    value = strtof(str, &endPtr);
    return (*endPtr == '\0' || *endPtr == ' ' || *endPtr == '\t');
  }
  
  // ----------------------------------------------------------------------------
  // Public Interface Implementation
  // ----------------------------------------------------------------------------
  
  bool initialize() {
    Serial.println("\n=== SerialInterface Module Initializing ===");
    
    // Clear command buffer
    memset(g_commandBuffer, 0, sizeof(g_commandBuffer));
    g_bufferIndex = 0;
    
    // Reset state
    g_initialized = true;
    g_lastStatusTime = millis();
    g_commandCounter = 0;
    
    Serial.println("SerialInterface: Initialization complete");
    Serial.println("Type 'HELP' for available commands");
    printPrompt();
    
    return true;
  }
  
  bool processConfigReset(const char* parameter) {
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (!config) {
      sendError("Configuration not available");
      return false;
    }
    
    String param = String(parameter);
    param.toLowerCase();
    
    if (param == "maxspeed") {
      MotionProfile profile = config->defaultProfile;
      profile.maxSpeed = DEFAULT_MAX_SPEED;
      if (SystemConfigMgr::setMotionProfile(profile) && SystemConfigMgr::commitChanges()) {
        sendInfo("Max speed reset to default");
        sendOK();
        return true;
      }
    }
    else if (param == "acceleration") {
      MotionProfile profile = config->defaultProfile;
      profile.acceleration = DEFAULT_ACCELERATION;
      if (SystemConfigMgr::setMotionProfile(profile) && SystemConfigMgr::commitChanges()) {
        sendInfo("Acceleration reset to default");
        sendOK();
        return true;
      }
    }
    else if (param == "deceleration") {
      MotionProfile profile = config->defaultProfile;
      profile.deceleration = DEFAULT_ACCELERATION;
      if (SystemConfigMgr::setMotionProfile(profile) && SystemConfigMgr::commitChanges()) {
        sendInfo("Deceleration reset to default");
        sendOK();
        return true;
      }
    }
    else if (param == "jerk") {
      MotionProfile profile = config->defaultProfile;
      profile.jerk = 1000.0f;
      if (SystemConfigMgr::setMotionProfile(profile) && SystemConfigMgr::commitChanges()) {
        sendInfo("Jerk reset to default");
        sendOK();
        return true;
      }
    }
    else if (param == "dmxstartchannel" || param == "dmxchannel") {
      if (SystemConfigMgr::setDMXConfig(DMX_START_CHANNEL, config->dmxScale, config->dmxOffset) && SystemConfigMgr::commitChanges()) {
        sendInfo("DMX start channel reset to default");
        sendOK();
        return true;
      }
    }
    else if (param == "dmxscale") {
      if (SystemConfigMgr::setDMXConfig(config->dmxStartChannel, 1.0f, config->dmxOffset) && SystemConfigMgr::commitChanges()) {
        sendInfo("DMX scale reset to default");
        sendOK();
        return true;
      }
    }
    else if (param == "dmxoffset") {
      if (SystemConfigMgr::setDMXConfig(config->dmxStartChannel, config->dmxScale, 0) && SystemConfigMgr::commitChanges()) {
        sendInfo("DMX offset reset to default");
        sendOK();
        return true;
      }
    }
    else if (param == "verbosity") {
      g_verbosityLevel = 2;
      sendInfo("Verbosity reset to default");
      sendOK();
      return true;
    }
    else if (param == "dmx") {
      if (SystemConfigMgr::setDMXConfig(DMX_START_CHANNEL, 1.0f, 0) && SystemConfigMgr::commitChanges()) {
        sendInfo("All DMX settings reset to defaults");
        sendOK();
        return true;
      }
    }
    else if (param == "motion") {
      MotionProfile profile = config->defaultProfile;
      profile.maxSpeed = DEFAULT_MAX_SPEED;
      profile.acceleration = DEFAULT_ACCELERATION;
      profile.deceleration = DEFAULT_ACCELERATION;
      profile.jerk = 1000.0f;
      if (SystemConfigMgr::setMotionProfile(profile) && SystemConfigMgr::commitChanges()) {
        sendInfo("All motion settings reset to defaults");
        sendOK();
        return true;
      }
    }
    else {
      sendError("Unknown parameter. Available: maxSpeed, acceleration, deceleration, jerk, dmxStartChannel, dmxScale, dmxOffset, verbosity, dmx, motion");
      return false;
    }
    
    sendError("Failed to reset parameter");
    return false;
  }
  
  bool processFactoryReset() {
    sendInfo("Performing factory reset...");
    
    if (SystemConfigMgr::resetToDefaults()) {
      // Reset interface settings too
      g_verbosityLevel = 2;
      g_echoMode = true;
      g_statusStreaming = false;
      g_jsonMode = false;
      
      sendInfo("Factory reset completed - all settings restored to defaults");
      sendOK();
      return true;
    } else {
      sendError("Factory reset failed");
      return false;
    }
  }
  
  bool update() {
    if (!g_initialized) return false;
    
    // Process incoming commands
    processIncomingCommands();
    
    // Send periodic status updates only if streaming is enabled
    SystemConfig* config = SystemConfigMgr::getConfig();
    uint32_t currentTime = millis();
    
    if (g_statusStreaming && config && config->enableSerialOutput && config->statusUpdateInterval > 0) {
      if (currentTime - g_lastStatusTime > config->statusUpdateInterval) {
        if (g_verbosityLevel >= 2) {
          if (g_jsonMode) {
            sendJSONStatus();
          } else {
            sendHumanStatus();
          }
        }
        g_lastStatusTime = currentTime;
      }
    }
    
    return true;
  }
  
  bool sendStatus() {
    return sendHumanStatus();
  }
  
  bool processIncomingCommands() {
    while (Serial.available()) {
      char c = Serial.read();
      
      // Echo character if enabled
      if (g_echoMode && c != '\r' && c != '\n') {
        Serial.print(c);
      }
      
      // Handle command completion
      if (c == '\n' || c == '\r') {
        if (g_echoMode) Serial.println();
        
        if (g_bufferIndex > 0) {
          g_commandBuffer[g_bufferIndex] = '\0';
          
          // Process the command
          if (g_commandBuffer[0] == '{') {
            // JSON command
            processJSONCommand(g_commandBuffer);
          } else {
            // Human command
            processHumanCommand(g_commandBuffer);
          }
          
          // Reset buffer
          g_bufferIndex = 0;
          memset(g_commandBuffer, 0, sizeof(g_commandBuffer));
        }
        
        printPrompt();
      }
      // Handle backspace
      else if (c == '\b' || c == 127) {
        if (g_bufferIndex > 0) {
          g_bufferIndex--;
          g_commandBuffer[g_bufferIndex] = '\0';
          if (g_echoMode) {
            Serial.print("\b \b");
          }
        }
      }
      // Add to buffer
      else if (g_bufferIndex < sizeof(g_commandBuffer) - 1) {
        g_commandBuffer[g_bufferIndex++] = c;
      }
    }
    
    return true;
  }
  
  bool sendResponse(const char* message) {
    Serial.println(message);
    return true;
  }
  
  // ----------------------------------------------------------------------------
  // Command Processing Functions
  // ----------------------------------------------------------------------------
  
  bool processHumanCommand(const char* command) {
    sendDebug("Processing human command");
    
    // Convert to uppercase for comparison
    String cmd = String(command);
    cmd.trim();
    cmd.toUpperCase();
    
    // Split command into tokens
    int spaceIndex = cmd.indexOf(' ');
    String mainCmd = (spaceIndex == -1) ? cmd : cmd.substring(0, spaceIndex);
    String params = (spaceIndex == -1) ? "" : cmd.substring(spaceIndex + 1);
    params.trim();
    
    // Process commands
    if (mainCmd == "HELP") {
      return sendHelp();
    }
    else if (mainCmd == "STATUS") {
      if (g_jsonMode) {
        return sendJSONStatus();
      } else {
        return sendHumanStatus();
      }
    }
    else if (mainCmd == "CONFIG") {
      if (params == "") {
        return sendJSONConfig();
      } else if (params.startsWith("SET ")) {
        // Parse CONFIG SET parameter value
        String setParams = params.substring(4);
        int valueIndex = setParams.indexOf(' ');
        if (valueIndex == -1) {
          sendError("CONFIG SET requires parameter and value");
          return false;
        }
        String param = setParams.substring(0, valueIndex);
        String value = setParams.substring(valueIndex + 1);
        return processConfigSet(param.c_str(), value.c_str());
      } else if (params.startsWith("RESET ")) {
        // Parse CONFIG RESET parameter
        String resetParam = params.substring(6);
        resetParam.trim();
        return processConfigReset(resetParam.c_str());
      } else if (params == "RESET") {
        // Factory reset all configuration
        return processFactoryReset();
      }
    }
    else if (mainCmd == "MOVE") {
      if (params == "") {
        sendError("MOVE requires position parameter");
        return false;
      }
      int32_t position;
      if (!parseInteger(params.c_str(), position)) {
        sendError("Invalid position value");
        return false;
      }
      MotionCommand cmd = createMotionCommand(CommandType::MOVE_ABSOLUTE, position);
      return sendMotionCommand(cmd);
    }
    else if (mainCmd == "HOME") {
      MotionCommand cmd = createMotionCommand(CommandType::HOME);
      return sendMotionCommand(cmd);
    }
    else if (mainCmd == "STOP") {
      MotionCommand cmd = createMotionCommand(CommandType::STOP);
      return sendMotionCommand(cmd);
    }
    else if (mainCmd == "ESTOP" || mainCmd == "EMERGENCY") {
      MotionCommand cmd = createMotionCommand(CommandType::EMERGENCY_STOP);
      return sendMotionCommand(cmd);
    }
    else if (mainCmd == "ENABLE") {
      MotionCommand cmd = createMotionCommand(CommandType::ENABLE);
      return sendMotionCommand(cmd);
    }
    else if (mainCmd == "DISABLE") {
      MotionCommand cmd = createMotionCommand(CommandType::DISABLE);
      return sendMotionCommand(cmd);
    }
    else if (mainCmd == "ECHO") {
      if (params == "ON" || params == "1") {
        g_echoMode = true;
        sendOK();
      } else if (params == "OFF" || params == "0") {
        g_echoMode = false;
        sendOK();
      } else {
        Serial.print("Echo mode: ");
        Serial.println(g_echoMode ? "ON" : "OFF");
      }
      return true;
    }
    else if (mainCmd == "VERBOSE") {
      if (params != "") {
        int32_t level;
        if (parseInteger(params.c_str(), level) && level >= 0 && level <= 3) {
          g_verbosityLevel = level;
          sendOK();
        } else {
          sendError("Verbosity level must be 0-3");
          return false;
        }
      } else {
        Serial.print("Verbosity level: ");
        Serial.println(g_verbosityLevel);
      }
      return true;
    }
    else if (mainCmd == "JSON") {
      if (params == "ON" || params == "1") {
        g_jsonMode = true;
        sendInfo("JSON output mode enabled");
        sendOK();
      } else if (params == "OFF" || params == "0") {
        g_jsonMode = false;
        sendInfo("JSON output mode disabled");
        sendOK();
      } else {
        if (g_jsonMode) {
          Serial.printf("{\"jsonMode\":%s}\n", g_jsonMode ? "true" : "false");
        } else {
          Serial.print("JSON output mode: ");
          Serial.println(g_jsonMode ? "ON" : "OFF");
        }
      }
      return true;
    }
    else if (mainCmd == "STREAM") {
      if (params == "ON" || params == "1") {
        g_statusStreaming = true;
        sendInfo("Status streaming enabled");
        sendOK();
      } else if (params == "OFF" || params == "0") {
        g_statusStreaming = false;
        sendInfo("Status streaming disabled");
        sendOK();
      } else {
        Serial.print("Status streaming: ");
        Serial.println(g_statusStreaming ? "ON" : "OFF");
      }
      return true;
    }
    else {
      sendError("Unknown command. Type HELP for available commands");
      return false;
    }
    
    return true;
  }
  
  bool processJSONCommand(const char* jsonCommand) {
    sendDebug("Processing JSON command");
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonCommand);
    
    if (error) {
      snprintf(g_responseBuffer, sizeof(g_responseBuffer), 
               "{\"status\":\"error\",\"message\":\"JSON parse error: %s\"}", 
               error.c_str());
      Serial.println(g_responseBuffer);
      return false;
    }
    
    if (!doc.containsKey("command")) {
      Serial.println("{\"status\":\"error\",\"message\":\"Missing command field\"}");
      return false;
    }
    
    String command = doc["command"].as<String>();
    command.toLowerCase();
    
    if (command == "status") {
      return sendJSONStatus();
    }
    else if (command == "config") {
      if (doc.containsKey("get")) {
        return sendJSONConfig();
      } else if (doc.containsKey("set")) {
        return processJSONConfigSet(doc["set"]);
      }
    }
    else if (command == "move") {
      if (!doc.containsKey("position")) {
        Serial.println("{\"status\":\"error\",\"message\":\"Missing position parameter\"}");
        return false;
      }
      int32_t position = doc["position"];
      MotionCommand cmd = createMotionCommand(CommandType::MOVE_ABSOLUTE, position);
      if (sendMotionCommand(cmd)) {
        Serial.println("{\"status\":\"ok\",\"message\":\"Move command queued\"}");
        return true;
      } else {
        Serial.println("{\"status\":\"error\",\"message\":\"Failed to queue move command\"}");
        return false;
      }
    }
    else if (command == "home") {
      MotionCommand cmd = createMotionCommand(CommandType::HOME);
      if (sendMotionCommand(cmd)) {
        Serial.println("{\"status\":\"ok\",\"message\":\"Home command queued\"}");
        return true;
      } else {
        Serial.println("{\"status\":\"error\",\"message\":\"Failed to queue home command\"}");
        return false;
      }
    }
    else if (command == "stop") {
      MotionCommand cmd = createMotionCommand(CommandType::STOP);
      if (sendMotionCommand(cmd)) {
        Serial.println("{\"status\":\"ok\",\"message\":\"Stop command queued\"}");
        return true;
      } else {
        Serial.println("{\"status\":\"error\",\"message\":\"Failed to queue stop command\"}");
        return false;
      }
    }
    else if (command == "enable") {
      MotionCommand cmd = createMotionCommand(CommandType::ENABLE);
      if (sendMotionCommand(cmd)) {
        Serial.println("{\"status\":\"ok\",\"message\":\"Enable command queued\"}");
        return true;
      } else {
        Serial.println("{\"status\":\"error\",\"message\":\"Failed to queue enable command\"}");
        return false;
      }
    }
    else if (command == "disable") {
      MotionCommand cmd = createMotionCommand(CommandType::DISABLE);
      if (sendMotionCommand(cmd)) {
        Serial.println("{\"status\":\"ok\",\"message\":\"Disable command queued\"}");
        return true;
      } else {
        Serial.println("{\"status\":\"error\",\"message\":\"Failed to queue disable command\"}");
        return false;
      }
    }
    else {
      Serial.println("{\"status\":\"error\",\"message\":\"Unknown JSON command\"}");
      return false;
    }
    
    return true;
  }
  
  bool processConfigSet(const char* parameter, const char* value) {
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (!config) {
      sendError("Configuration not available");
      return false;
    }
    
    String param = String(parameter);
    param.toLowerCase();
    
    if (param == "maxspeed") {
      float speed;
      if (!parseFloat(value, speed)) {
        sendError("Invalid speed value");
        return false;
      }
      sendDebug("Setting max speed");
      MotionProfile profile = config->defaultProfile;
      profile.maxSpeed = speed;
      if (SystemConfigMgr::setMotionProfile(profile)) {
        if (SystemConfigMgr::commitChanges()) {
          sendInfo("Max speed updated successfully");
          sendOK();
          return true;
        } else {
          sendError("Failed to save max speed to flash");
          return false;
        }
      } else {
        sendError("Invalid speed value (must be 0-10000 steps/sec)");
        return false;
      }
    }
    else if (param == "acceleration") {
      float accel;
      if (!parseFloat(value, accel)) {
        sendError("Invalid acceleration value");
        return false;
      }
      sendDebug("Setting acceleration");
      MotionProfile profile = config->defaultProfile;
      profile.acceleration = accel;
      if (SystemConfigMgr::setMotionProfile(profile)) {
        if (SystemConfigMgr::commitChanges()) {
          sendInfo("Acceleration updated successfully");
          sendOK();
          return true;
        } else {
          sendError("Failed to save acceleration to flash");
          return false;
        }
      } else {
        sendError("Invalid acceleration value (must be 0-20000 steps/sec²)");
        return false;
      }
    }
    else if (param == "verbosity") {
      int32_t level;
      if (!parseInteger(value, level) || level < 0 || level > 3) {
        sendError("Verbosity must be 0-3");
        return false;
      }
      g_verbosityLevel = level;
      sendOK();
      return true;
    }
    else if (param == "dmxstartchannel") {
      int32_t channel;
      if (!parseInteger(value, channel) || channel < 1 || channel > 512) {
        sendError("DMX start channel must be 1-512");
        return false;
      }
      sendDebug("Setting DMX start channel");
      if (SystemConfigMgr::setDMXConfig(channel, config->dmxScale, config->dmxOffset)) {
        if (SystemConfigMgr::commitChanges()) {
          sendInfo("DMX start channel updated successfully");
          sendOK();
          return true;
        } else {
          sendError("Failed to save DMX start channel to flash");
          return false;
        }
      } else {
        sendError("Invalid DMX channel value");
        return false;
      }
    }
    else if (param == "dmxscale") {
      float scale;
      if (!parseFloat(value, scale) || scale == 0.0f) {
        sendError("DMX scale cannot be zero");
        return false;
      }
      sendDebug("Setting DMX scale");
      if (SystemConfigMgr::setDMXConfig(config->dmxStartChannel, scale, config->dmxOffset)) {
        if (SystemConfigMgr::commitChanges()) {
          sendInfo("DMX scale updated successfully");
          sendOK();
          return true;
        } else {
          sendError("Failed to save DMX scale to flash");
          return false;
        }
      } else {
        sendError("Invalid DMX scale value");
        return false;
      }
    }
    else if (param == "dmxoffset") {
      int32_t offset;
      if (!parseInteger(value, offset)) {
        sendError("Invalid DMX offset value");
        return false;
      }
      sendDebug("Setting DMX offset");
      if (SystemConfigMgr::setDMXConfig(config->dmxStartChannel, config->dmxScale, offset)) {
        if (SystemConfigMgr::commitChanges()) {
          sendInfo("DMX offset updated successfully");
          sendOK();
          return true;
        } else {
          sendError("Failed to save DMX offset to flash");
          return false;
        }
      } else {
        sendError("Invalid DMX offset value");
        return false;
      }
    }
    else {
      sendError("Unknown configuration parameter");
      return false;
    }
  }
  
  bool processJSONConfigSet(ArduinoJson::JsonObject setObj) {
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (!config) {
      Serial.println("{\"status\":\"error\",\"message\":\"Configuration not available\"}");
      return false;
    }
    
    bool configChanged = false;
    MotionProfile profile = config->defaultProfile;
    
    // Process motion profile changes
    if (setObj.containsKey("maxSpeed")) {
      profile.maxSpeed = setObj["maxSpeed"];
      configChanged = true;
    }
    if (setObj.containsKey("acceleration")) {
      profile.acceleration = setObj["acceleration"];
      configChanged = true;
    }
    if (setObj.containsKey("deceleration")) {
      profile.deceleration = setObj["deceleration"];
      configChanged = true;
    }
    if (setObj.containsKey("jerk")) {
      profile.jerk = setObj["jerk"];
      configChanged = true;
    }
    
    // Apply motion profile changes
    if (configChanged) {
      if (!SystemConfigMgr::setMotionProfile(profile)) {
        Serial.println("{\"status\":\"error\",\"message\":\"Invalid motion profile parameters\"}");
        return false;
      }
    }
    
    // Process other configuration changes
    if (setObj.containsKey("dmxStartChannel")) {
      uint16_t channel = setObj["dmxStartChannel"];
      if (!SystemConfigMgr::setDMXConfig(channel, config->dmxScale, config->dmxOffset)) {
        Serial.println("{\"status\":\"error\",\"message\":\"Invalid DMX start channel\"}");
        return false;
      }
      configChanged = true;
    }
    
    if (setObj.containsKey("dmxScale")) {
      float scale = setObj["dmxScale"];
      if (!SystemConfigMgr::setDMXConfig(config->dmxStartChannel, scale, config->dmxOffset)) {
        Serial.println("{\"status\":\"error\",\"message\":\"Invalid DMX scale\"}");
        return false;
      }
      configChanged = true;
    }
    
    if (setObj.containsKey("dmxOffset")) {
      int32_t offset = setObj["dmxOffset"];
      if (!SystemConfigMgr::setDMXConfig(config->dmxStartChannel, config->dmxScale, offset)) {
        Serial.println("{\"status\":\"error\",\"message\":\"Invalid DMX offset\"}");
        return false;
      }
      configChanged = true;
    }
    
    // Commit changes if any were made
    if (configChanged) {
      if (SystemConfigMgr::commitChanges()) {
        Serial.println("{\"status\":\"ok\",\"message\":\"Configuration updated\"}");
        return true;
      } else {
        Serial.println("{\"status\":\"error\",\"message\":\"Failed to save configuration\"}");
        return false;
      }
    }
    
    Serial.println("{\"status\":\"ok\",\"message\":\"No configuration changes\"}");
    return true;
  }
  
  bool setEchoMode(bool enable) {
    g_echoMode = enable;
    return true;
  }
  
  bool setVerbosity(uint8_t level) {
    if (level <= 3) {
      g_verbosityLevel = level;
      return true;
    }
    return false;
  }
  
  // ----------------------------------------------------------------------------
  // Status Reporting Functions
  // ----------------------------------------------------------------------------
  
  bool sendHumanStatus() {
    Serial.println("\n=== System Status ===");
    
    // System state
    SystemState state = getSystemState();
    Serial.print("System State: ");
    switch (state) {
      case SystemState::UNINITIALIZED: Serial.println("UNINITIALIZED"); break;
      case SystemState::INITIALIZING:  Serial.println("INITIALIZING");  break;
      case SystemState::READY:         Serial.println("READY");         break;
      case SystemState::RUNNING:       Serial.println("RUNNING");       break;
      case SystemState::STOPPING:      Serial.println("STOPPING");      break;
      case SystemState::STOPPED:       Serial.println("STOPPED");       break;
      case SystemState::ERROR:         Serial.println("ERROR");         break;
      case SystemState::EMERGENCY_STOP: Serial.println("EMERGENCY_STOP"); break;
    }
    
    // Motion information
    int32_t currentPos, targetPos;
    float currentSpeed;
    bool stepperEnabled;
    
    SAFE_READ_STATUS(currentPosition, currentPos);
    SAFE_READ_STATUS(targetPosition, targetPos);
    SAFE_READ_STATUS(currentSpeed, currentSpeed);
    SAFE_READ_STATUS(stepperEnabled, stepperEnabled);
    
    Serial.printf("Position: %d steps (target: %d)\n", currentPos, targetPos);
    Serial.printf("Speed: %.1f steps/sec\n", currentSpeed);
    Serial.printf("Stepper: %s\n", stepperEnabled ? "ENABLED" : "DISABLED");
    
    // Configuration summary
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (config) {
      Serial.printf("Max Speed: %.1f steps/sec\n", config->defaultProfile.maxSpeed);
      Serial.printf("Acceleration: %.1f steps/sec²\n", config->defaultProfile.acceleration);
      Serial.printf("DMX Channel: %d\n", config->dmxStartChannel);
      Serial.printf("DMX Scale: %.2f steps/unit\n", config->dmxScale);
      Serial.printf("DMX Offset: %d steps\n", config->dmxOffset);
    }
    
    Serial.printf("Uptime: %lu ms\n", getSystemUptime());
    Serial.println("=====================\n");
    
    return true;
  }
  
  bool sendJSONStatus() {
    StaticJsonDocument<512> doc;
    
    // System state
    SystemState state = getSystemState();
    doc["systemState"] = (int)state;
    
    // Motion information
    int32_t currentPos, targetPos;
    float currentSpeed;
    bool stepperEnabled;
    
    SAFE_READ_STATUS(currentPosition, currentPos);
    SAFE_READ_STATUS(targetPosition, targetPos);
    SAFE_READ_STATUS(currentSpeed, currentSpeed);
    SAFE_READ_STATUS(stepperEnabled, stepperEnabled);
    
    doc["position"]["current"] = currentPos;
    doc["position"]["target"] = targetPos;
    doc["speed"] = currentSpeed;
    doc["stepperEnabled"] = stepperEnabled;
    doc["uptime"] = getSystemUptime();
    
    // Configuration summary
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (config) {
      doc["config"]["maxSpeed"] = config->defaultProfile.maxSpeed;
      doc["config"]["acceleration"] = config->defaultProfile.acceleration;
      doc["config"]["dmxChannel"] = config->dmxStartChannel;
      doc["config"]["dmxScale"] = config->dmxScale;
      doc["config"]["dmxOffset"] = config->dmxOffset;
    }
    
    serializeJson(doc, Serial);
    Serial.println();
    
    return true;
  }
  
  bool sendJSONConfig() {
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (!config) {
      Serial.println("{\"status\":\"error\",\"message\":\"Configuration not available\"}");
      return false;
    }
    
    // Create comprehensive JSON output with metadata
    StaticJsonDocument<2048> doc;
    
    // Current configuration values
    doc["config"]["motion"]["maxSpeed"]["value"] = config->defaultProfile.maxSpeed;
    doc["config"]["motion"]["maxSpeed"]["min"] = 0.0;
    doc["config"]["motion"]["maxSpeed"]["max"] = 10000.0;
    doc["config"]["motion"]["maxSpeed"]["units"] = "steps/sec";
    doc["config"]["motion"]["maxSpeed"]["description"] = "Maximum velocity";
    
    doc["config"]["motion"]["acceleration"]["value"] = config->defaultProfile.acceleration;
    doc["config"]["motion"]["acceleration"]["min"] = 0.0;
    doc["config"]["motion"]["acceleration"]["max"] = 20000.0;
    doc["config"]["motion"]["acceleration"]["units"] = "steps/sec²";
    doc["config"]["motion"]["acceleration"]["description"] = "Acceleration rate";
    
    doc["config"]["motion"]["deceleration"]["value"] = config->defaultProfile.deceleration;
    doc["config"]["motion"]["deceleration"]["min"] = 0.0;
    doc["config"]["motion"]["deceleration"]["max"] = 20000.0;
    doc["config"]["motion"]["deceleration"]["units"] = "steps/sec²";
    doc["config"]["motion"]["deceleration"]["description"] = "Deceleration rate";
    
    doc["config"]["motion"]["jerk"]["value"] = config->defaultProfile.jerk;
    doc["config"]["motion"]["jerk"]["min"] = 0.0;
    doc["config"]["motion"]["jerk"]["max"] = 50000.0;
    doc["config"]["motion"]["jerk"]["units"] = "steps/sec³";
    doc["config"]["motion"]["jerk"]["description"] = "Jerk limitation";
    
    doc["config"]["motion"]["targetPosition"]["value"] = config->defaultProfile.targetPosition;
    doc["config"]["motion"]["targetPosition"]["units"] = "steps";
    doc["config"]["motion"]["targetPosition"]["description"] = "Current target position";
    
    doc["config"]["motion"]["enableLimits"]["value"] = config->defaultProfile.enableLimits;
    doc["config"]["motion"]["enableLimits"]["description"] = "Respect limit switches during motion";
    
    // Position configuration
    doc["config"]["position"]["homePosition"]["value"] = config->homePosition;
    doc["config"]["position"]["homePosition"]["units"] = "steps";
    doc["config"]["position"]["homePosition"]["description"] = "Home reference position";
    
    doc["config"]["position"]["minPosition"]["value"] = config->minPosition;
    doc["config"]["position"]["minPosition"]["units"] = "steps";
    doc["config"]["position"]["minPosition"]["description"] = "Minimum allowed position";
    
    doc["config"]["position"]["maxPosition"]["value"] = config->maxPosition;
    doc["config"]["position"]["maxPosition"]["units"] = "steps";
    doc["config"]["position"]["maxPosition"]["description"] = "Maximum allowed position";
    
    doc["config"]["position"]["range"]["value"] = config->maxPosition - config->minPosition;
    doc["config"]["position"]["range"]["min"] = 100;
    doc["config"]["position"]["range"]["units"] = "steps";
    doc["config"]["position"]["range"]["description"] = "Position range (must be >= 100 steps)";
    
    // DMX configuration
    doc["config"]["dmx"]["startChannel"]["value"] = config->dmxStartChannel;
    doc["config"]["dmx"]["startChannel"]["min"] = 1;
    doc["config"]["dmx"]["startChannel"]["max"] = 512;
    doc["config"]["dmx"]["startChannel"]["description"] = "DMX channel to monitor";
    
    doc["config"]["dmx"]["scale"]["value"] = config->dmxScale;
    doc["config"]["dmx"]["scale"]["min"] = -1000.0;
    doc["config"]["dmx"]["scale"]["max"] = 1000.0;
    doc["config"]["dmx"]["scale"]["units"] = "steps/DMX_unit";
    doc["config"]["dmx"]["scale"]["description"] = "Position scaling factor (cannot be 0)";
    doc["config"]["dmx"]["scale"]["constraint"] = "non-zero";
    
    doc["config"]["dmx"]["offset"]["value"] = config->dmxOffset;
    doc["config"]["dmx"]["offset"]["units"] = "steps";
    doc["config"]["dmx"]["offset"]["description"] = "Position offset applied after scaling";
    
    doc["config"]["dmx"]["timeout"]["value"] = config->dmxTimeout;
    doc["config"]["dmx"]["timeout"]["min"] = 100;
    doc["config"]["dmx"]["timeout"]["max"] = 60000;
    doc["config"]["dmx"]["timeout"]["units"] = "milliseconds";
    doc["config"]["dmx"]["timeout"]["description"] = "DMX signal timeout";
    
    // Safety configuration
    doc["config"]["safety"]["enableLimitSwitches"]["value"] = config->enableLimitSwitches;
    doc["config"]["safety"]["enableLimitSwitches"]["description"] = "Monitor limit switch inputs";
    
    doc["config"]["safety"]["enableStepperAlarm"]["value"] = config->enableStepperAlarm;
    doc["config"]["safety"]["enableStepperAlarm"]["description"] = "Monitor stepper driver alarm signal";
    
    doc["config"]["safety"]["emergencyDeceleration"]["value"] = config->emergencyDeceleration;
    doc["config"]["safety"]["emergencyDeceleration"]["min"] = 100.0;
    doc["config"]["safety"]["emergencyDeceleration"]["max"] = 50000.0;
    doc["config"]["safety"]["emergencyDeceleration"]["units"] = "steps/sec²";
    doc["config"]["safety"]["emergencyDeceleration"]["description"] = "Emergency stop deceleration rate";
    
    // System configuration
    doc["config"]["system"]["statusUpdateInterval"]["value"] = config->statusUpdateInterval;
    doc["config"]["system"]["statusUpdateInterval"]["min"] = 10;
    doc["config"]["system"]["statusUpdateInterval"]["max"] = 10000;
    doc["config"]["system"]["statusUpdateInterval"]["units"] = "milliseconds";
    doc["config"]["system"]["statusUpdateInterval"]["description"] = "Status update frequency";
    
    doc["config"]["system"]["enableSerialOutput"]["value"] = config->enableSerialOutput;
    doc["config"]["system"]["enableSerialOutput"]["description"] = "Enable serial status output";
    
    doc["config"]["system"]["serialVerbosity"]["value"] = config->serialVerbosity;
    doc["config"]["system"]["serialVerbosity"]["min"] = 0;
    doc["config"]["system"]["serialVerbosity"]["max"] = 3;
    doc["config"]["system"]["serialVerbosity"]["description"] = "Serial output verbosity level";
    doc["config"]["system"]["serialVerbosity"]["options"] = "0=minimal, 1=normal, 2=verbose, 3=debug";
    
    // Metadata
    doc["config"]["version"]["value"] = config->configVersion;
    doc["config"]["version"]["description"] = "Configuration version";
    
    doc["metadata"]["timestamp"] = millis();
    doc["metadata"]["source"] = "SkullStepperV4";
    doc["metadata"]["version"] = "4.0.0";
    
    // Hardware constants for reference
    doc["hardware"]["stepperStepsPerRev"] = STEPPER_STEPS_PER_REV;
    doc["hardware"]["stepperMicrosteps"] = STEPPER_MICROSTEPS;  
    doc["hardware"]["totalStepsPerRev"] = TOTAL_STEPS_PER_REV;
    doc["hardware"]["minStepInterval"]["value"] = MIN_STEP_INTERVAL;
    doc["hardware"]["minStepInterval"]["units"] = "microseconds";
    doc["hardware"]["maxStepFrequency"]["value"] = 1000000 / MIN_STEP_INTERVAL;
    doc["hardware"]["maxStepFrequency"]["units"] = "Hz";
    
    serializeJsonPretty(doc, Serial);
    Serial.println();
    
    return true;
  }
  
  bool sendHelp() {
    Serial.println("\n=== SkullStepperV4 Commands ===");
    Serial.println("Motion Commands:");
    Serial.println("  MOVE <position>     - Move to absolute position");
    Serial.println("  HOME                - Start homing sequence");
    Serial.println("  STOP                - Stop current motion");
    Serial.println("  ESTOP               - Emergency stop");
    Serial.println("  ENABLE              - Enable stepper motor");
    Serial.println("  DISABLE             - Disable stepper motor");
    Serial.println();
    Serial.println("Information Commands:");
    Serial.println("  STATUS              - Show system status");
    Serial.println("  CONFIG              - Show configuration");
    Serial.println("  CONFIG SET <param> <value> - Set configuration");
    Serial.println("  HELP                - Show this help");
    Serial.println();
    Serial.println("Interface Commands:");
    Serial.println("  ECHO ON/OFF         - Enable/disable command echo");
    Serial.println("  VERBOSE <0-3>       - Set verbosity level");
    Serial.println();
    Serial.println("JSON Commands:");
    Serial.println("  {\"command\":\"move\",\"position\":1000}");
    Serial.println("  {\"command\":\"status\"}");
    Serial.println("  {\"command\":\"config\",\"get\":\"all\"}");
    Serial.println("  {\"command\":\"config\",\"set\":{\"maxSpeed\":2000}}");
    Serial.println("  {\"command\":\"config\",\"set\":{\"dmxStartChannel\":10}}");
    Serial.println("  {\"command\":\"config\",\"set\":{\"dmxScale\":10.0,\"dmxOffset\":500}}");
    Serial.println();
    Serial.println("Examples:");
    Serial.println("  MOVE 1000           - Move to position 1000");
    Serial.println("  CONFIG SET maxSpeed 2000 - Set max speed");
    Serial.println("===============================\n");
    
    return true;
  }
  
  // ----------------------------------------------------------------------------
  // Motion Command Functions
  // ----------------------------------------------------------------------------
  
  bool sendMotionCommand(const MotionCommand& cmd) {
    if (g_motionCommandQueue == NULL) {
      sendError("Motion command queue not available");
      return false;
    }
    
    // Try to send command to queue (non-blocking)
    if (xQueueSend(g_motionCommandQueue, &cmd, 0) == pdTRUE) {
      sendInfo("Motion command queued");
      return true;
    } else {
      sendError("Motion command queue full");
      return false;
    }
  }
  
  MotionCommand createMotionCommand(CommandType type, int32_t target, float speed) {
    MotionCommand cmd = {};
    cmd.type = type;
    cmd.timestamp = millis();
    cmd.commandId = ++g_commandCounter;
    
    // Get current motion profile from config
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (config) {
      cmd.profile = config->defaultProfile;
    }
    
    // Override target position if specified
    if (target != 0 || type == CommandType::MOVE_ABSOLUTE) {
      cmd.profile.targetPosition = target;
    }
    
    // Override speed if specified
    if (speed > 0) {
      cmd.profile.maxSpeed = speed;
    }
    
    return cmd;
  }
}