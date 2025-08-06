// ============================================================================
// File: SystemConfig.cpp
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.0.0
// Date: 2025-01-23
// Author: Tim Rosener
// Description: SystemConfig module implementation - ESP32 flash-based storage
// License: MIT
// Phase: 2 (Complete) - Configuration Management
// ============================================================================

#include "SystemConfig.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// ============================================================================
// SystemConfig Module - Phase 2 Flash-Based Implementation
// ============================================================================

namespace SystemConfigMgr {
  
  // Flash storage configuration
  const char* CONFIG_NAMESPACE = "skullstepper";
  const uint32_t CONFIG_VERSION = 0x00020001; // Version 2.1
  
  // Internal state
  static bool g_configLoaded = false;
  static bool g_configValid = false;
  static Preferences g_preferences;
  
  // ============================================================================
  // Private Helper Functions
  // ============================================================================
  
  void setDefaultConfiguration() {
    Serial.println("SystemConfig: Setting default configuration");
    
    // Clear and initialize the global config
    memset(&g_systemConfig, 0, sizeof(SystemConfig));
    
    // Motion profile defaults
    g_systemConfig.defaultProfile.maxSpeed = DEFAULT_MAX_SPEED;
    g_systemConfig.defaultProfile.acceleration = DEFAULT_ACCELERATION;
    g_systemConfig.defaultProfile.deceleration = DEFAULT_ACCELERATION;
    g_systemConfig.defaultProfile.jerk = 1000.0f;
    g_systemConfig.defaultProfile.targetPosition = 0;
    g_systemConfig.defaultProfile.enableLimits = true;
    
    // Position limits
    g_systemConfig.homePositionPercent = 50.0f;  // Default to center of range
    g_systemConfig.minPosition = MIN_POSITION_STEPS;
    g_systemConfig.maxPosition = MAX_POSITION_STEPS;
    g_systemConfig.homingSpeed = 940.0f;  // Default homing speed (steps/sec)
    g_systemConfig.autoHomeOnBoot = false;  // Default: manual homing required
    g_systemConfig.autoHomeOnEstop = false;  // Default: manual homing after E-stop
    
    // DMX configuration
    g_systemConfig.dmxStartChannel = DMX_START_CHANNEL;
    g_systemConfig.dmxScale = 1.0f;
    g_systemConfig.dmxOffset = 0;
    g_systemConfig.dmxTimeout = 5000;
    
    // Safety configuration
    g_systemConfig.enableLimitSwitches = true;
    g_systemConfig.enableStepperAlarm = true;
    g_systemConfig.emergencyDeceleration = EMERGENCY_STOP_DECEL;
    
    // System configuration
    g_systemConfig.statusUpdateInterval = STATUS_UPDATE_INTERVAL_MS;
    g_systemConfig.enableSerialOutput = true;
    g_systemConfig.serialVerbosity = 2;
    g_systemConfig.configVersion = CONFIG_VERSION;
    
    Serial.println("SystemConfig: Default configuration loaded");
  }
  
  bool readConfigFromFlash() {
    Serial.println("SystemConfig: Reading configuration from flash");
    
    if (!g_preferences.begin(CONFIG_NAMESPACE, true)) { // read-only
      Serial.println("SystemConfig: Failed to open preferences");
      return false;
    }
    
    // Check if configuration exists and version matches
    uint32_t storedVersion = g_preferences.getUInt("version", 0);
    if (storedVersion != CONFIG_VERSION) {
      Serial.printf("SystemConfig: Version mismatch or no config. Stored: 0x%08X, Expected: 0x%08X\n", 
                    storedVersion, CONFIG_VERSION);
      g_preferences.end();
      return false;
    }
    
    // Load motion profile
    g_systemConfig.defaultProfile.maxSpeed = g_preferences.getFloat("maxSpeed", DEFAULT_MAX_SPEED);
    g_systemConfig.defaultProfile.acceleration = g_preferences.getFloat("acceleration", DEFAULT_ACCELERATION);
    g_systemConfig.defaultProfile.deceleration = g_preferences.getFloat("deceleration", DEFAULT_ACCELERATION);
    g_systemConfig.defaultProfile.jerk = g_preferences.getFloat("jerk", 1000.0f);
    g_systemConfig.defaultProfile.targetPosition = g_preferences.getInt("targetPos", 0);
    g_systemConfig.defaultProfile.enableLimits = g_preferences.getBool("enableLimits", true);
    
    // Load position limits
    g_systemConfig.homePositionPercent = g_preferences.getFloat("homePosPercent", 50.0f);
    g_systemConfig.minPosition = g_preferences.getInt("minPos", MIN_POSITION_STEPS);
    g_systemConfig.maxPosition = g_preferences.getInt("maxPos", MAX_POSITION_STEPS);
    g_systemConfig.homingSpeed = g_preferences.getFloat("homingSpeed", 940.0f);
    g_systemConfig.autoHomeOnBoot = g_preferences.getBool("autoHomeOnBoot", false);
    g_systemConfig.autoHomeOnEstop = g_preferences.getBool("autoHomeOnEstop", false);
    
    // Load DMX configuration
    g_systemConfig.dmxStartChannel = g_preferences.getUShort("dmxChannel", DMX_START_CHANNEL);
    g_systemConfig.dmxScale = g_preferences.getFloat("dmxScale", 1.0f);
    g_systemConfig.dmxOffset = g_preferences.getInt("dmxOffset", 0);
    g_systemConfig.dmxTimeout = g_preferences.getUInt("dmxTimeout", 5000);
    
    // Load safety configuration
    g_systemConfig.enableLimitSwitches = g_preferences.getBool("limitSwitches", true);
    g_systemConfig.enableStepperAlarm = g_preferences.getBool("stepperAlarm", true);
    g_systemConfig.emergencyDeceleration = g_preferences.getFloat("emergencyDecel", EMERGENCY_STOP_DECEL);
    
    // Load system configuration
    g_systemConfig.statusUpdateInterval = g_preferences.getUInt("statusInterval", STATUS_UPDATE_INTERVAL_MS);
    g_systemConfig.enableSerialOutput = g_preferences.getBool("serialOutput", true);
    g_systemConfig.serialVerbosity = g_preferences.getUChar("verbosity", 2);
    g_systemConfig.configVersion = storedVersion;
    
    g_preferences.end();
    
    // Display loaded configuration
    Serial.println("SystemConfig: *** Configuration Loaded from Flash ***");
    Serial.printf("  Motion Profile:\n");
    Serial.printf("    Max Speed: %.1f steps/sec\n", g_systemConfig.defaultProfile.maxSpeed);
    Serial.printf("    Acceleration: %.1f steps/sec²\n", g_systemConfig.defaultProfile.acceleration);
    Serial.printf("    Deceleration: %.1f steps/sec²\n", g_systemConfig.defaultProfile.deceleration);
    Serial.printf("    Jerk: %.1f steps/sec³\n", g_systemConfig.defaultProfile.jerk);
    Serial.printf("    Target Position: %d steps\n", g_systemConfig.defaultProfile.targetPosition);
    Serial.printf("    Enable Limits: %s\n", g_systemConfig.defaultProfile.enableLimits ? "ON" : "OFF");
    
    Serial.printf("  Position Settings:\n");
    Serial.printf("    Home Position: %.1f%% of range\n", g_systemConfig.homePositionPercent);
    Serial.printf("    Min Position: %d steps\n", g_systemConfig.minPosition);
    Serial.printf("    Max Position: %d steps\n", g_systemConfig.maxPosition);
    Serial.printf("    Homing Speed: %.1f steps/sec\n", g_systemConfig.homingSpeed);
    Serial.printf("    Auto-Home on Boot: %s\n", g_systemConfig.autoHomeOnBoot ? "ON" : "OFF");
    Serial.printf("    Auto-Home on E-Stop: %s\n", g_systemConfig.autoHomeOnEstop ? "ON" : "OFF");
    
    Serial.printf("  DMX Configuration:\n");
    Serial.printf("    Start Channel: %d\n", g_systemConfig.dmxStartChannel);
    Serial.printf("    Scale Factor: %.3f\n", g_systemConfig.dmxScale);
    Serial.printf("    Offset: %d steps\n", g_systemConfig.dmxOffset);
    Serial.printf("    Timeout: %d ms\n", g_systemConfig.dmxTimeout);
    
    Serial.printf("  Safety Configuration:\n");
    Serial.printf("    Limit Switches: %s\n", g_systemConfig.enableLimitSwitches ? "ON" : "OFF");
    Serial.printf("    Stepper Alarm: %s\n", g_systemConfig.enableStepperAlarm ? "ON" : "OFF");
    Serial.printf("    Emergency Decel: %.1f steps/sec²\n", g_systemConfig.emergencyDeceleration);
    
    Serial.printf("  System Configuration:\n");
    Serial.printf("    Status Update Interval: %d ms\n", g_systemConfig.statusUpdateInterval);
    Serial.printf("    Serial Output: %s\n", g_systemConfig.enableSerialOutput ? "ON" : "OFF");
    Serial.printf("    Serial Verbosity: %d\n", g_systemConfig.serialVerbosity);
    Serial.printf("    Config Version: 0x%08X\n", g_systemConfig.configVersion);
    
    return true;
  }
  
  bool writeConfigToFlash() {
    Serial.println("SystemConfig: Saving configuration to flash");
    
    if (!g_preferences.begin(CONFIG_NAMESPACE, false)) { // read-write
      Serial.println("SystemConfig: Failed to open preferences for writing");
      return false;
    }
    
    // Save version first
    g_preferences.putUInt("version", CONFIG_VERSION);
    
    // Save motion profile
    g_preferences.putFloat("maxSpeed", g_systemConfig.defaultProfile.maxSpeed);
    g_preferences.putFloat("acceleration", g_systemConfig.defaultProfile.acceleration);
    g_preferences.putFloat("deceleration", g_systemConfig.defaultProfile.deceleration);
    g_preferences.putFloat("jerk", g_systemConfig.defaultProfile.jerk);
    g_preferences.putInt("targetPos", g_systemConfig.defaultProfile.targetPosition);
    g_preferences.putBool("enableLimits", g_systemConfig.defaultProfile.enableLimits);
    
    // Save position limits
    g_preferences.putFloat("homePosPercent", g_systemConfig.homePositionPercent);
    g_preferences.putInt("minPos", g_systemConfig.minPosition);
    g_preferences.putInt("maxPos", g_systemConfig.maxPosition);
    g_preferences.putFloat("homingSpeed", g_systemConfig.homingSpeed);
    g_preferences.putBool("autoHomeOnBoot", g_systemConfig.autoHomeOnBoot);
    g_preferences.putBool("autoHomeOnEstop", g_systemConfig.autoHomeOnEstop);
    
    // Save DMX configuration
    g_preferences.putUShort("dmxChannel", g_systemConfig.dmxStartChannel);
    g_preferences.putFloat("dmxScale", g_systemConfig.dmxScale);
    g_preferences.putInt("dmxOffset", g_systemConfig.dmxOffset);
    g_preferences.putUInt("dmxTimeout", g_systemConfig.dmxTimeout);
    
    // Save safety configuration
    g_preferences.putBool("limitSwitches", g_systemConfig.enableLimitSwitches);
    g_preferences.putBool("stepperAlarm", g_systemConfig.enableStepperAlarm);
    g_preferences.putFloat("emergencyDecel", g_systemConfig.emergencyDeceleration);
    
    // Save system configuration
    g_preferences.putUInt("statusInterval", g_systemConfig.statusUpdateInterval);
    g_preferences.putBool("serialOutput", g_systemConfig.enableSerialOutput);
    g_preferences.putUChar("verbosity", g_systemConfig.serialVerbosity);
    
    g_preferences.end();
    
    Serial.println("SystemConfig: Configuration saved to flash");
    return true;
  }
  
  // ============================================================================
  // Public Interface Implementation
  // ============================================================================
  
  bool initialize() {
    Serial.println("SystemConfig: Initializing with flash storage...");
    
    // Try to load configuration from flash
    if (readConfigFromFlash()) {
      g_configLoaded = true;
      g_configValid = true;
      Serial.println("SystemConfig: Loaded existing configuration from flash");
    } else {
      // Load defaults if flash config is invalid
      Serial.println("SystemConfig: Loading default configuration");
      setDefaultConfiguration();
      g_configLoaded = true;
      g_configValid = true;
      
      // Save defaults to flash
      if (!writeConfigToFlash()) {
        Serial.println("SystemConfig: Warning - Failed to save defaults to flash");
      }
    }
    
    return true;
  }
  
  bool loadFromEEPROM() {
    // Legacy function - now loads from flash
    return readConfigFromFlash();
  }
  
  bool saveToEEPROM() {
    // Legacy function - now saves to flash
    return writeConfigToFlash();
  }
  
  bool validateConfig() {
    // Validate motion profile
    if (!validateMotionProfile(g_systemConfig.defaultProfile)) {
      return false;
    }
    
    // Validate position limits
    if (!validatePositionLimits(g_systemConfig.minPosition, g_systemConfig.maxPosition)) {
      return false;
    }
    
    // Validate DMX configuration
    if (!validateDMXConfig(g_systemConfig.dmxStartChannel, g_systemConfig.dmxScale, g_systemConfig.dmxOffset)) {
      return false;
    }
    
    // Validate safety settings
    if (g_systemConfig.emergencyDeceleration <= 0) {
      Serial.println("SystemConfig: Invalid emergency deceleration");
      return false;
    }
    
    // Validate timeouts
    if (g_systemConfig.dmxTimeout == 0 || g_systemConfig.statusUpdateInterval == 0) {
      Serial.println("SystemConfig: Invalid timeout values");
      return false;
    }
    
    g_configValid = true;
    return true;
  }
  
  bool resetToDefaults() {
    Serial.println("SystemConfig: Resetting to factory defaults");
    
    // Clear flash storage
    if (g_preferences.begin(CONFIG_NAMESPACE, false)) {
      g_preferences.clear();
      g_preferences.end();
    }
    
    setDefaultConfiguration();
    g_configValid = true;
    return writeConfigToFlash();
  }
  
  SystemConfig* getConfig() {
    return &g_systemConfig;
  }
  
  bool commitChanges() {
    if (!validateConfig()) {
      Serial.println("SystemConfig: Configuration validation failed");
      return false;
    }
    
    return writeConfigToFlash();
  }
  
  // ============================================================================
  // Parameter Access Functions
  // ============================================================================
  
  bool setMotionProfile(const MotionProfile& profile) {
    if (!validateMotionProfile(profile)) {
      return false;
    }
    
    SAFE_WRITE_CONFIG(defaultProfile, profile);
    return true;
  }
  
  MotionProfile getMotionProfile() {
    MotionProfile profile;
    SAFE_READ_CONFIG(defaultProfile, profile);
    return profile;
  }
  
  bool setPositionLimits(int32_t minPos, int32_t maxPos) {
    if (!validatePositionLimits(minPos, maxPos)) {
      return false;
    }
    
    SAFE_WRITE_CONFIG(minPosition, minPos);
    SAFE_WRITE_CONFIG(maxPosition, maxPos);
    return true;
  }
  
  bool setDMXConfig(uint16_t startChannel, float scale, int32_t offset) {
    if (!validateDMXConfig(startChannel, scale, offset)) {
      return false;
    }
    
    SAFE_WRITE_CONFIG(dmxStartChannel, startChannel);
    SAFE_WRITE_CONFIG(dmxScale, scale);
    SAFE_WRITE_CONFIG(dmxOffset, offset);
    return true;
  }
  
  bool setSafetyConfig(bool enableLimits, bool enableAlarm, float emergencyDecel) {
    if (emergencyDecel <= 0) {
      Serial.println("SystemConfig: Invalid emergency deceleration");
      return false;
    }
    
    SAFE_WRITE_CONFIG(enableLimitSwitches, enableLimits);
    SAFE_WRITE_CONFIG(enableStepperAlarm, enableAlarm);
    SAFE_WRITE_CONFIG(emergencyDeceleration, emergencyDecel);
    return true;
  }
  
  // ============================================================================
  // Parameter Validation Functions
  // ============================================================================
  
  bool validateMotionProfile(const MotionProfile& profile) {
    if (profile.maxSpeed <= 0 || profile.maxSpeed > 10000) {
      Serial.printf("SystemConfig: Invalid max speed: %.2f\n", profile.maxSpeed);
      return false;
    }
    
    if (profile.acceleration <= 0 || profile.acceleration > 20000) {
      Serial.printf("SystemConfig: Invalid acceleration: %.2f\n", profile.acceleration);
      return false;
    }
    
    if (profile.deceleration <= 0 || profile.deceleration > 20000) {
      Serial.printf("SystemConfig: Invalid deceleration: %.2f\n", profile.deceleration);
      return false;
    }
    
    if (profile.jerk <= 0 || profile.jerk > 50000) {
      Serial.printf("SystemConfig: Invalid jerk: %.2f\n", profile.jerk);
      return false;
    }
    
    return true;
  }
  
  bool validatePositionLimits(int32_t minPos, int32_t maxPos) {
    if (minPos >= maxPos) {
      Serial.printf("SystemConfig: Invalid position limits: min=%d, max=%d\n", minPos, maxPos);
      return false;
    }
    
    if (maxPos - minPos < 100) {
      Serial.println("SystemConfig: Position range too small (minimum 100 steps)");
      return false;
    }
    
    return true;
  }
  
  bool validateHomePositionPercent(float percent) {
    if (percent < 0.0f || percent > 100.0f) {
      Serial.printf("SystemConfig: Invalid home position percentage: %.1f%%\n", percent);
      return false;
    }
    return true;
  }
  
  bool validateDMXConfig(uint16_t startChannel, float scale, int32_t offset) {
    if (startChannel < 1 || startChannel > 512) {
      Serial.printf("SystemConfig: Invalid DMX start channel: %d\n", startChannel);
      return false;
    }
    
    if (scale == 0.0f) {
      Serial.println("SystemConfig: Invalid DMX scale (cannot be zero)");
      return false;
    }
    
    return true;
  }
  
  // ============================================================================
  // JSON Export/Import Functions
  // ============================================================================
  
  size_t exportToJSON(char* buffer, size_t bufferSize) {
    StaticJsonDocument<1024> doc;
    
    // Motion profile
    doc["motion"]["maxSpeed"] = g_systemConfig.defaultProfile.maxSpeed;
    doc["motion"]["acceleration"] = g_systemConfig.defaultProfile.acceleration;
    doc["motion"]["deceleration"] = g_systemConfig.defaultProfile.deceleration;
    doc["motion"]["jerk"] = g_systemConfig.defaultProfile.jerk;
    doc["motion"]["enableLimits"] = g_systemConfig.defaultProfile.enableLimits;
    
    // Position limits
    doc["position"]["homePositionPercent"] = g_systemConfig.homePositionPercent;
    doc["position"]["minPosition"] = g_systemConfig.minPosition;
    doc["position"]["maxPosition"] = g_systemConfig.maxPosition;
    doc["position"]["homingSpeed"] = g_systemConfig.homingSpeed;
    
    // DMX configuration
    doc["dmx"]["startChannel"] = g_systemConfig.dmxStartChannel;
    doc["dmx"]["scale"] = g_systemConfig.dmxScale;
    doc["dmx"]["offset"] = g_systemConfig.dmxOffset;
    doc["dmx"]["timeout"] = g_systemConfig.dmxTimeout;
    
    // Safety configuration
    doc["safety"]["enableLimitSwitches"] = g_systemConfig.enableLimitSwitches;
    doc["safety"]["enableStepperAlarm"] = g_systemConfig.enableStepperAlarm;
    doc["safety"]["emergencyDeceleration"] = g_systemConfig.emergencyDeceleration;
    
    // System configuration
    doc["system"]["statusUpdateInterval"] = g_systemConfig.statusUpdateInterval;
    doc["system"]["enableSerialOutput"] = g_systemConfig.enableSerialOutput;
    doc["system"]["serialVerbosity"] = g_systemConfig.serialVerbosity;
    doc["system"]["configVersion"] = g_systemConfig.configVersion;
    
    return serializeJson(doc, buffer, bufferSize);
  }
  
  bool importFromJSON(const char* jsonString) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
      Serial.printf("SystemConfig: JSON parse error: %s\n", error.c_str());
      return false;
    }
    
    // Create temporary config for validation
    SystemConfig tempConfig = g_systemConfig;
    
    // Import motion profile
    if (doc.containsKey("motion")) {
      tempConfig.defaultProfile.maxSpeed = doc["motion"]["maxSpeed"] | tempConfig.defaultProfile.maxSpeed;
      tempConfig.defaultProfile.acceleration = doc["motion"]["acceleration"] | tempConfig.defaultProfile.acceleration;
      tempConfig.defaultProfile.deceleration = doc["motion"]["deceleration"] | tempConfig.defaultProfile.deceleration;
      tempConfig.defaultProfile.jerk = doc["motion"]["jerk"] | tempConfig.defaultProfile.jerk;
      tempConfig.defaultProfile.enableLimits = doc["motion"]["enableLimits"] | tempConfig.defaultProfile.enableLimits;
    }
    
    // Import position limits
    if (doc.containsKey("position")) {
      tempConfig.homePositionPercent = doc["position"]["homePositionPercent"] | tempConfig.homePositionPercent;
      tempConfig.minPosition = doc["position"]["minPosition"] | tempConfig.minPosition;
      tempConfig.maxPosition = doc["position"]["maxPosition"] | tempConfig.maxPosition;
      tempConfig.homingSpeed = doc["position"]["homingSpeed"] | tempConfig.homingSpeed;
    }
    
    // Import DMX configuration
    if (doc.containsKey("dmx")) {
      tempConfig.dmxStartChannel = doc["dmx"]["startChannel"] | tempConfig.dmxStartChannel;
      tempConfig.dmxScale = doc["dmx"]["scale"] | tempConfig.dmxScale;
      tempConfig.dmxOffset = doc["dmx"]["offset"] | tempConfig.dmxOffset;
      tempConfig.dmxTimeout = doc["dmx"]["timeout"] | tempConfig.dmxTimeout;
    }
    
    // Import safety configuration
    if (doc.containsKey("safety")) {
      tempConfig.enableLimitSwitches = doc["safety"]["enableLimitSwitches"] | tempConfig.enableLimitSwitches;
      tempConfig.enableStepperAlarm = doc["safety"]["enableStepperAlarm"] | tempConfig.enableStepperAlarm;
      tempConfig.emergencyDeceleration = doc["safety"]["emergencyDeceleration"] | tempConfig.emergencyDeceleration;
    }
    
    // Import system configuration
    if (doc.containsKey("system")) {
      tempConfig.statusUpdateInterval = doc["system"]["statusUpdateInterval"] | tempConfig.statusUpdateInterval;
      tempConfig.enableSerialOutput = doc["system"]["enableSerialOutput"] | tempConfig.enableSerialOutput;
      tempConfig.serialVerbosity = doc["system"]["serialVerbosity"] | tempConfig.serialVerbosity;
    }
    
    // Validate imported configuration
    if (!validateMotionProfile(tempConfig.defaultProfile) ||
        !validatePositionLimits(tempConfig.minPosition, tempConfig.maxPosition) ||
        !validateDMXConfig(tempConfig.dmxStartChannel, tempConfig.dmxScale, tempConfig.dmxOffset)) {
      Serial.println("SystemConfig: Imported JSON configuration failed validation");
      return false;
    }
    
    // Apply validated configuration
    if (xSemaphoreTake(g_configMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      memcpy(&g_systemConfig, &tempConfig, sizeof(SystemConfig));
      xSemaphoreGive(g_configMutex);
      g_configValid = true;
      Serial.println("SystemConfig: Configuration imported from JSON");
      return true;
    }
    
    return false;
  }
  
  uint16_t getChecksum() {
    // Flash storage doesn't use checksums - return version instead
    return (uint16_t)(g_systemConfig.configVersion & 0xFFFF);
  }
  
  uint32_t getVersion() {
    return g_systemConfig.configVersion;
  }
  
  // ============================================================================
  // Flash Management Functions (Legacy EEPROM Interface)
  // ============================================================================
  
  bool isEEPROMValid() {
    // Check if preferences exist
    if (!g_preferences.begin(CONFIG_NAMESPACE, true)) {
      return false;
    }
    
    uint32_t version = g_preferences.getUInt("version", 0);
    g_preferences.end();
    
    return (version == CONFIG_VERSION);
  }
  
  bool eraseEEPROM() {
    Serial.println("SystemConfig: Erasing flash configuration");
    
    if (!g_preferences.begin(CONFIG_NAMESPACE, false)) {
      return false;
    }
    
    bool result = g_preferences.clear();
    g_preferences.end();
    
    return result;
  }
  
  bool getEEPROMStats(size_t& totalBytes, size_t& usedBytes) {
    // Flash usage stats are not easily available
    totalBytes = 4096;  // Estimated flash partition size
    usedBytes = 256;    // Estimated usage
    return true;
  }
}