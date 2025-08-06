 
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
#include "StepperController.h"
#include <ArduinoJson.h>
#include <esp_random.h>

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
  
  // Range test state
  static bool g_rangeTestActive = false;
  static int32_t g_testPos1 = 0;
  static int32_t g_testPos2 = 0;
  static bool g_testMovingToPos2 = true;
  static uint32_t g_testMoveCount = 0;
  static uint32_t g_lastTestCheckTime = 0;
  
  // Random test state
  static bool g_randomTestActive = false;
  static int32_t g_randomPositions[10];
  static uint8_t g_randomTestIndex = 0;
  static uint32_t g_randomTestMoveCount = 0;
  
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
  
  // Start range test between two positions
  bool startRangeTest(int32_t pos1, int32_t pos2) {
    g_rangeTestActive = true;
    g_testPos1 = pos1;
    g_testPos2 = pos2;
    g_testMovingToPos2 = true;
    g_testMoveCount = 0;
    g_lastTestCheckTime = millis();
    
    // Send first move command
    MotionCommand cmd = createMotionCommand(CommandType::MOVE_ABSOLUTE, pos2);
    return sendMotionCommand(cmd);
  }
  
  // Start random position test
  bool startRandomTest(int32_t minPos, int32_t maxPos) {
    g_randomTestActive = true;
    g_randomTestIndex = 0;
    g_randomTestMoveCount = 0;
    g_lastTestCheckTime = millis();
    
    // Generate 10 random positions within the range
    int32_t range = maxPos - minPos;
    Serial.println("INFO: Generated random test positions:");
    for (int i = 0; i < 10; i++) {
      // Use ESP32 hardware random number generator
      g_randomPositions[i] = minPos + (esp_random() % range);
      Serial.printf("  Position %d: %d steps\n", i + 1, g_randomPositions[i]);
    }
    
    // Send first move command
    MotionCommand cmd = createMotionCommand(CommandType::MOVE_ABSOLUTE, g_randomPositions[0]);
    return sendMotionCommand(cmd);
  }
  
  // Update range test (called from update())
  void updateRangeTest() {
    if (!g_rangeTestActive) return;
    
    // Check if limit fault is active - stop test if so
    if (StepperController::isLimitFaultActive()) {
      g_rangeTestActive = false;
      sendError("Range test aborted - limit fault detected. Homing required.");
      return;
    }
    
    // Check motion state every 100ms
    uint32_t currentTime = millis();
    if (currentTime - g_lastTestCheckTime < 100) {
      return;
    }
    g_lastTestCheckTime = currentTime;
    
    // Check if stepper is moving
    if (!StepperController::isMoving()) {
      // Movement completed, send next move
      g_testMoveCount++;
      
      // Toggle direction
      int32_t targetPos = g_testMovingToPos2 ? g_testPos1 : g_testPos2;
      g_testMovingToPos2 = !g_testMovingToPos2;
      
      // Show progress every 10 moves
      if (g_testMoveCount % 10 == 0) {
        Serial.printf("INFO: Test cycle %lu completed\n", g_testMoveCount / 2);
      }
      
      // Send next move
      MotionCommand cmd = createMotionCommand(CommandType::MOVE_ABSOLUTE, targetPos);
      sendMotionCommand(cmd);
    }
  }
  
  // Update random test (called from update())
  void updateRandomTest() {
    if (!g_randomTestActive) return;
    
    // Check if limit fault is active - stop test if so
    if (StepperController::isLimitFaultActive()) {
      g_randomTestActive = false;
      sendError("Random test aborted - limit fault detected. Homing required.");
      return;
    }
    
    // Check motion state every 100ms
    uint32_t currentTime = millis();
    if (currentTime - g_lastTestCheckTime < 100) {
      return;
    }
    g_lastTestCheckTime = currentTime;
    
    // Check if stepper is moving
    if (!StepperController::isMoving()) {
      // Movement completed
      g_randomTestMoveCount++;
      
      // Check if we've completed all 10 positions
      if (g_randomTestIndex >= 9) {
        // Test complete
        g_randomTestActive = false;
        Serial.printf("INFO: Random test complete - visited %d positions\n", g_randomTestMoveCount);
        Serial.println("INFO: Test finished successfully");
        return;
      }
      
      // Move to next position
      g_randomTestIndex++;
      int32_t targetPos = g_randomPositions[g_randomTestIndex];
      
      Serial.printf("INFO: Moving to position %d of 10: %d steps\n", 
                    g_randomTestIndex + 1, targetPos);
      
      // Send next move
      MotionCommand cmd = createMotionCommand(CommandType::MOVE_ABSOLUTE, targetPos);
      sendMotionCommand(cmd);
    }
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
    else if (param == "homingspeed") {
      config->homingSpeed = 940.0f;  // Default homing speed
      if (SystemConfigMgr::commitChanges()) {
        sendInfo("Homing speed reset to default");
        sendOK();
        return true;
      }
    }
    else if (param == "homepositionpercent" || param == "homepercent") {
      config->homePositionPercent = 50.0f;  // Default to center
      if (SystemConfigMgr::commitChanges()) {
        sendInfo("Home position percentage reset to default (50%)");
        sendOK();
        return true;
      }
    }
    else if (param == "autohomeonboot") {
      config->autoHomeOnBoot = false;  // Default: off
      if (SystemConfigMgr::commitChanges()) {
        sendInfo("Auto-home on boot reset to default (OFF)");
        sendOK();
        return true;
      }
    }
    else if (param == "autohomeonestop") {
      config->autoHomeOnEstop = false;  // Default: off
      if (SystemConfigMgr::commitChanges()) {
        sendInfo("Auto-home on E-stop reset to default (OFF)");
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
      config->homingSpeed = 940.0f;
      if (SystemConfigMgr::setMotionProfile(profile) && SystemConfigMgr::commitChanges()) {
        sendInfo("All motion settings reset to defaults");
        sendOK();
        return true;
      }
    }
    else {
      sendError("Unknown parameter. Available: maxSpeed, acceleration, deceleration, jerk, homingSpeed, homePositionPercent, autoHomeOnBoot, autoHomeOnEstop, dmxStartChannel, dmxScale, dmxOffset, verbosity, dmx, motion");
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
    
    // Update range test if active
    updateRangeTest();
    
    // Update random test if active
    updateRandomTest();
    
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
      
      // If range test is active, any input stops it
      if (g_rangeTestActive) {
        // Stop the test
        g_rangeTestActive = false;
        sendInfo("Range test stopped by user");
        
        // Stop motion
        MotionCommand cmd = createMotionCommand(CommandType::STOP);
        sendMotionCommand(cmd);
        
        // Clear the buffer and show prompt
        g_bufferIndex = 0;
        memset(g_commandBuffer, 0, sizeof(g_commandBuffer));
        printPrompt();
        continue;
      }
      
      // If random test is active, any input stops it
      if (g_randomTestActive) {
        // Stop the test
        g_randomTestActive = false;
        sendInfo("Random test stopped by user");
        
        // Stop motion
        MotionCommand cmd = createMotionCommand(CommandType::STOP);
        sendMotionCommand(cmd);
        
        // Clear the buffer and show prompt
        g_bufferIndex = 0;
        memset(g_commandBuffer, 0, sizeof(g_commandBuffer));
        printPrompt();
        continue;
      }
      
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
    else if (mainCmd == "MOVEHOME" || mainCmd == "GOTOHOME") {
      // Check if system is homed
      if (!StepperController::isHomed()) {
        sendError("System must be homed before moving to home position");
        return false;
      }
      
      // Get position limits and home percentage
      int32_t minPos, maxPos;
      if (!StepperController::getPositionLimits(minPos, maxPos)) {
        sendError("Unable to get position limits");
        return false;
      }
      
      SystemConfig* config = SystemConfigMgr::getConfig();
      if (!config) {
        sendError("Configuration not available");
        return false;
      }
      
      // Calculate home position based on percentage
      int32_t range = maxPos - minPos;
      int32_t homePosition = minPos + (int32_t)((range * config->homePositionPercent) / 100.0f);
      
      sendInfo("Moving to home position");
      Serial.printf("INFO: Target position: %d (%.1f%% of range)\n", homePosition, config->homePositionPercent);
      
      MotionCommand cmd = createMotionCommand(CommandType::MOVE_ABSOLUTE, homePosition);
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
    else if (mainCmd == "PARAMS") {
      return sendParameterList();
    }
    else if (mainCmd == "DIAG") {
      if (params == "ON" || params == "1") {
        StepperController::enableStepDiagnostics(true);
        sendOK();
      } else if (params == "OFF" || params == "0") {
        StepperController::enableStepDiagnostics(false);
        sendOK();
      } else {
        sendError("DIAG requires ON or OFF parameter");
        return false;
      }
      return true;
    }
    else if (mainCmd == "TEST") {
      // Check if system has been homed
      if (!StepperController::isHomed()) {
        sendError("System must be homed before running test. Use HOME command first.");
        return false;
      }
      
      // Get the user-configured position limits from SystemConfig
      SystemConfig* config = SystemConfigMgr::getConfig();
      if (!config) {
        sendError("Configuration not available");
        return false;
      }
      
      int32_t minPos = config->minPosition;
      int32_t maxPos = config->maxPosition;
      
      // Validate that user limits are reasonable
      if (maxPos <= minPos || (maxPos - minPos) < 100) {
        sendError("Invalid user-configured position limits");
        return false;
      }
      
      // Calculate 10% and 90% positions of user-configured range
      int32_t range = maxPos - minPos;
      int32_t pos10 = minPos + (range * 10 / 100);
      int32_t pos90 = minPos + (range * 90 / 100);
      
      sendInfo("Starting range test...");
      Serial.printf("INFO: Moving between positions %d (10%%) and %d (90%%) of user-configured range\n", pos10, pos90);
      Serial.printf("INFO: User limits: %d to %d steps\n", minPos, maxPos);
      Serial.println("INFO: Press any key to stop test");
      
      // Start test sequence
      return startRangeTest(pos10, pos90);
    }
    else if (mainCmd == "TEST2" || mainCmd == "RANDOMTEST") {
      // Check if system has been homed
      if (!StepperController::isHomed()) {
        sendError("System must be homed before running test. Use HOME command first.");
        return false;
      }
      
      // Get the user-configured position limits from SystemConfig
      SystemConfig* config = SystemConfigMgr::getConfig();
      if (!config) {
        sendError("Configuration not available");
        return false;
      }
      
      int32_t minPos = config->minPosition;
      int32_t maxPos = config->maxPosition;
      
      // Validate that user limits are reasonable
      if (maxPos <= minPos || (maxPos - minPos) < 100) {
        sendError("Invalid user-configured position limits");
        return false;
      }
      
      // Use 10% to 90% of user-configured range for safety
      int32_t range = maxPos - minPos;
      int32_t safeMin = minPos + (range * 10 / 100);
      int32_t safeMax = minPos + (range * 90 / 100);
      
      sendInfo("Starting random position test...");
      Serial.printf("INFO: Will move to 10 random positions between %d and %d (user-configured range)\n", safeMin, safeMax);
      Serial.printf("INFO: User limits: %d to %d steps\n", minPos, maxPos);
      Serial.println("INFO: Press any key to stop test");
      
      // Start random test sequence
      return startRandomTest(safeMin, safeMax);
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
        // For config get command, just return the config
        return sendJSONConfig();
      } else if (doc.containsKey("set")) {
        return processJSONConfigSet(doc["set"]);
      } else {
        // No get or set specified, just return config
        return sendJSONConfig();
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
          
          // Send SET_SPEED command to update StepperController's internal profile
          MotionCommand cmd = createMotionCommand(CommandType::SET_SPEED);
          cmd.profile.maxSpeed = speed;
          if (!sendMotionCommand(cmd)) {
            sendDebug("Warning: Failed to update stepper controller speed");
          }
          
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
          
          // Send SET_ACCELERATION command to update StepperController's internal profile
          MotionCommand cmd = createMotionCommand(CommandType::SET_ACCELERATION);
          cmd.profile.acceleration = accel;
          if (!sendMotionCommand(cmd)) {
            sendDebug("Warning: Failed to update stepper controller acceleration");
          }
          
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
    else if (param == "homingspeed") {
      float speed;
      if (!parseFloat(value, speed) || speed <= 0 || speed > 10000) {
        sendError("Invalid homing speed value (must be 0-10000 steps/sec)");
        return false;
      }
      sendDebug("Setting homing speed");
      config->homingSpeed = speed;
      if (SystemConfigMgr::commitChanges()) {
        sendInfo("Homing speed updated successfully");
        sendOK();
        return true;
      } else {
        sendError("Failed to save homing speed to flash");
        return false;
      }
    }
    else if (param == "homepositionpercent" || param == "homepercent") {
      float percent;
      if (!parseFloat(value, percent) || percent < 0 || percent > 100) {
        sendError("Invalid home position percentage (must be 0-100%)");
        return false;
      }
      sendDebug("Setting home position percentage");
      config->homePositionPercent = percent;
      if (SystemConfigMgr::commitChanges()) {
        sendInfo("Home position percentage updated successfully");
        sendOK();
        return true;
      } else {
        sendError("Failed to save home position percentage to flash");
        return false;
      }
    }
    else if (param == "autohomeonboot") {
      bool enabled = (String(value).equalsIgnoreCase("true") || String(value) == "1" || String(value).equalsIgnoreCase("on"));
      sendDebug("Setting auto-home on boot");
      config->autoHomeOnBoot = enabled;
      if (SystemConfigMgr::commitChanges()) {
        sendInfo("Auto-home on boot updated successfully");
        sendOK();
        return true;
      } else {
        sendError("Failed to save auto-home on boot to flash");
        return false;
      }
    }
    else if (param == "autohomeonestop") {
      bool enabled = (String(value).equalsIgnoreCase("true") || String(value) == "1" || String(value).equalsIgnoreCase("on"));
      sendDebug("Setting auto-home on E-stop");
      config->autoHomeOnEstop = enabled;
      if (SystemConfigMgr::commitChanges()) {
        sendInfo("Auto-home on E-stop updated successfully");
        sendOK();
        return true;
      } else {
        sendError("Failed to save auto-home on E-stop to flash");
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
      
      // Send commands to update StepperController's internal profile
      if (setObj.containsKey("maxSpeed")) {
        MotionCommand cmd = createMotionCommand(CommandType::SET_SPEED);
        cmd.profile.maxSpeed = profile.maxSpeed;
        sendMotionCommand(cmd);
      }
      if (setObj.containsKey("acceleration")) {
        MotionCommand cmd = createMotionCommand(CommandType::SET_ACCELERATION);
        cmd.profile.acceleration = profile.acceleration;
        sendMotionCommand(cmd);
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
    
    // Show homing status
    if (!StepperController::isHomed()) {
        Serial.println("\n*** SYSTEM NOT HOMED - MOVEMENT DISABLED ***");
        Serial.println("Use HOME command to establish position limits");
    } else {
        // Show position limits if homed
        int32_t minPos, maxPos;
        if (StepperController::getPositionLimits(minPos, maxPos)) {
            Serial.printf("Position Limits: %d to %d steps (range: %d)\n", 
                         minPos, maxPos, maxPos - minPos);
        }
    }
    
    // Show limit fault status
    if (StepperController::isLimitFaultActive()) {
        Serial.println("\n*** LIMIT FAULT ACTIVE - HOMING REQUIRED ***");
        Serial.println("Unexpected limit switch activation detected");
    }
    
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
    doc["isHomed"] = StepperController::isHomed();
    doc["limitFaultActive"] = StepperController::isLimitFaultActive();
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
    doc["config"]["position"]["homePosition"]["value"] = 0;  // Always 0 at left limit
    doc["config"]["position"]["homePosition"]["units"] = "steps";
    doc["config"]["position"]["homePosition"]["description"] = "Reference position at left limit (always 0)";
    
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
    
    doc["config"]["position"]["homingSpeed"]["value"] = config->homingSpeed;
    doc["config"]["position"]["homingSpeed"]["min"] = 0.0;
    doc["config"]["position"]["homingSpeed"]["max"] = 10000.0;
    doc["config"]["position"]["homingSpeed"]["units"] = "steps/sec";
    doc["config"]["position"]["homingSpeed"]["description"] = "Speed used during homing sequence";
    
    doc["config"]["position"]["homePositionPercent"]["value"] = config->homePositionPercent;
    doc["config"]["position"]["homePositionPercent"]["min"] = 0.0;
    doc["config"]["position"]["homePositionPercent"]["max"] = 100.0;
    doc["config"]["position"]["homePositionPercent"]["units"] = "%";
    doc["config"]["position"]["homePositionPercent"]["description"] = "Position to return to after homing (percentage of range)";
    
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
    Serial.println("  MOVEHOME            - Move to configured home position");
    Serial.println("  HOME                - Start auto-range homing sequence:");
    Serial.println("                        1. Find left limit & set as home (0)");
    Serial.println("                        2. Find right limit to determine range");
    Serial.println("                        3. Set operating bounds with safety margins");
    Serial.println("                        4. Move to center of detected range");
    Serial.println("  STOP                - Stop current motion");
    Serial.println("  ESTOP               - Emergency stop");
    Serial.println("  ENABLE              - Enable stepper motor");
    Serial.println("  DISABLE             - Disable stepper motor");
    Serial.println("  TEST                - Run range test (requires homing first)");
    Serial.println("                        Moves between 10% and 90% of range");
    Serial.println("                        Press any key to stop");
    Serial.println("  TEST2 / RANDOMTEST  - Run random position test");
    Serial.println("                        Moves to 10 random positions");
    Serial.println("                        Press any key to stop");
    Serial.println("  DIAG ON/OFF         - Enable/disable step timing diagnostics");
    Serial.println();
    Serial.println("Information Commands:");
    Serial.println("  STATUS              - Show system status");
    Serial.println("  CONFIG              - Show configuration");
    Serial.println("  CONFIG SET <param> <value> - Set configuration");
    Serial.println("  PARAMS              - List all configurable parameters");
    Serial.println("  HELP                - Show this help");
    Serial.println();
    Serial.println("Interface Commands:");
    Serial.println("  ECHO ON/OFF         - Enable/disable command echo");
    Serial.println("  VERBOSE <0-3>       - Set verbosity level");
  Serial.println("  JSON ON/OFF         - Switch output mode");
  Serial.println("  STREAM ON/OFF       - Auto status updates");
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
  
  bool sendParameterList() {
    Serial.println("\n=== Configurable Parameters ===");
    Serial.println("\nMotion Parameters:");
    Serial.println("  maxSpeed            Range: 0-10000 steps/sec    Default: 1000");
    Serial.println("                      Current max velocity for movements");
    Serial.println("  acceleration        Range: 0-20000 steps/sec²   Default: 500");
    Serial.println("                      Acceleration/deceleration rate");
    Serial.println("  deceleration        Range: 0-20000 steps/sec²   Default: 500");
    Serial.println("                      (Currently uses same value as acceleration)");
    Serial.println("  jerk                Range: 0-50000 steps/sec³   Default: 1000");
    Serial.println("                      Jerk limitation (future use)");
    Serial.println("  homingSpeed         Range: 0-10000 steps/sec    Default: 940");
    Serial.println("                      Speed used during homing sequence");
    Serial.println("  homePositionPercent Range: 0-100 %              Default: 50");
    Serial.println("                      Position to return to after homing (% of range)");
    Serial.println("  autoHomeOnBoot      Boolean: true/false         Default: false");
    Serial.println("                      Automatically home on system startup");
    Serial.println("  autoHomeOnEstop     Boolean: true/false         Default: false");
    Serial.println("                      Automatically home after E-stop/limit fault");
    
    Serial.println("\nDMX Parameters:");
    Serial.println("  dmxStartChannel     Range: 1-512                Default: 1");
    Serial.println("                      DMX channel to monitor for position control");
    Serial.println("  dmxScale            Range: Any non-zero value   Default: 1.0");
    Serial.println("                      Scaling factor (steps per DMX unit)");
    Serial.println("                      Negative values reverse direction");
    Serial.println("  dmxOffset           Range: Any integer          Default: 0");
    Serial.println("                      Position offset in steps");
    Serial.println("                      Final position = (DMX × scale) + offset");
    
    Serial.println("\nSystem Parameters:");
    Serial.println("  verbosity           Range: 0-3                  Default: 2");
    Serial.println("                      0=minimal, 1=normal, 2=verbose, 3=debug");
    
    Serial.println("\nUsage Examples:");
    Serial.println("  CONFIG SET maxSpeed 2000        # Set max speed to 2000 steps/sec");
    Serial.println("  CONFIG SET acceleration 1500    # Set acceleration to 1500 steps/sec²");
    Serial.println("  CONFIG SET homingSpeed 1500     # Set homing speed to 1500 steps/sec");
    Serial.println("  CONFIG SET homePositionPercent 75  # Return to 75% of range after homing");
    Serial.println("  CONFIG SET autoHomeOnBoot true  # Enable auto-homing on startup");
    Serial.println("  CONFIG SET autoHomeOnEstop on   # Enable auto-homing after E-stop");
    Serial.println("  CONFIG SET dmxStartChannel 10   # Monitor DMX channel 10");
    Serial.println("  CONFIG SET dmxScale 5.0         # 5 steps per DMX unit");
    Serial.println("  CONFIG SET dmxOffset 1000       # Add 1000 steps offset");
    
    Serial.println("\nReset Commands:");
    Serial.println("  CONFIG RESET <parameter>        # Reset single parameter");
    Serial.println("  CONFIG RESET motion             # Reset all motion parameters");
    Serial.println("  CONFIG RESET dmx                # Reset all DMX parameters");
    Serial.println("  CONFIG RESET                    # Factory reset all parameters");
    
    Serial.println("\nNote: Position limits are set automatically during homing.");
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
    
    // Check for limit fault before queuing motion commands
    if (StepperController::isLimitFaultActive() && 
        (cmd.type == CommandType::MOVE_ABSOLUTE || 
         cmd.type == CommandType::MOVE_RELATIVE)) {
      // Don't spam the queue with commands that will be rejected
      // The StepperController will log the rejection once
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
    
    // For SET_SPEED and SET_ACCELERATION commands, ensure the profile is included
    // This ensures config changes are applied to the stepper controller
    if (type == CommandType::SET_SPEED || type == CommandType::SET_ACCELERATION) {
      // Profile already contains the updated values from SystemConfig
    }
    
    return cmd;
  }
}