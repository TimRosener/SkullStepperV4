// ============================================================================
// File: DMXReceiver.cpp
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.1.2
// Date: 2025-02-02
// Author: Tim Rosener
// Description: DMXReceiver module implementation - DMX512 signal reception
// License: MIT
// Phase: 6 (Active Development) - Phases 1-4 Implementation
// ============================================================================

#include "DMXReceiver.h"
#include "StepperController.h"
#include "SystemConfig.h"
#include <ESP32S3DMX.h>
#include <Arduino.h>

// ============================================================================
// DMXReceiver Module - Phase 6 Implementation with ESP32S3DMX
// ============================================================================

namespace DMXReceiver {
  
  // ----------------------------------------------------------------------------
  // Private Module Variables
  // ----------------------------------------------------------------------------
  
  // DMX receiver instance
  static ESP32S3DMX dmx;
  
  // Module state
  static DMXState currentState = DMXState::NO_SIGNAL;
  static bool moduleInitialized = false;
  
  // Channel configuration
  static uint16_t baseChannel = 1;  // Default base channel
  static const uint8_t NUM_CHANNELS = 5;  // We use 5 consecutive channels
  static uint8_t channelCache[NUM_CHANNELS] = {0};  // Cache for our channels
  
  // Signal tracking
  static bool dmxConnected = false;
  static uint32_t lastPacketTime = 0;
  static uint32_t signalTimeout = 1000;  // Default 1 second timeout
  
  // Statistics
  static uint32_t totalPackets = 0;
  static uint32_t errorPackets = 0;
  static uint32_t lastPacketCount = 0;
  
  // Task handle for Core 0 execution
  static TaskHandle_t dmxTaskHandle = NULL;
  
  // DMX control state
  static DMXMode currentMode = DMXMode::STOP;
  static DMXMode lastMode = DMXMode::STOP;
  static const uint8_t MODE_HYSTERESIS = 5;  // Prevent mode flickering
  
  // DMX configuration
  static bool dmxEnabled = true;  // DMX control enabled by default
  static bool use16BitPosition = false;  // Default to 8-bit mode
  static uint32_t lastMotionCommandTime = 0;
  static int32_t lastTargetPosition = -1;  // Track last commanded position
  
  // ----------------------------------------------------------------------------
  // DMX Mode Detection with Hysteresis
  // ----------------------------------------------------------------------------
  
  static DMXMode detectModeWithHysteresis(uint8_t modeValue) {
    DMXMode detectedMode;
    
    // Determine raw mode from value
    if (modeValue <= MODE_STOP_MAX) {
      detectedMode = DMXMode::STOP;
    } else if (modeValue <= MODE_CONTROL_MAX) {
      detectedMode = DMXMode::CONTROL;
    } else {
      detectedMode = DMXMode::HOME;
    }
    
    // Apply hysteresis if mode is different from current
    if (detectedMode != currentMode) {
      // Check if we're far enough from the boundary
      switch (currentMode) {
        case DMXMode::STOP:
          if (detectedMode == DMXMode::CONTROL && modeValue < (MODE_STOP_MAX + MODE_HYSTERESIS)) {
            return currentMode;  // Stay in STOP mode
          }
          break;
          
        case DMXMode::CONTROL:
          if (detectedMode == DMXMode::STOP && modeValue > (MODE_STOP_MAX - MODE_HYSTERESIS)) {
            return currentMode;  // Stay in CONTROL mode
          }
          if (detectedMode == DMXMode::HOME && modeValue < (MODE_CONTROL_MAX + MODE_HYSTERESIS)) {
            return currentMode;  // Stay in CONTROL mode
          }
          break;
          
        case DMXMode::HOME:
          if (detectedMode == DMXMode::CONTROL && modeValue > (MODE_CONTROL_MAX - MODE_HYSTERESIS)) {
            return currentMode;  // Stay in HOME mode
          }
          break;
      }
    }
    
    return detectedMode;
  }
  
  // ----------------------------------------------------------------------------
  // Process DMX Channels and Generate Motion Commands
  // ----------------------------------------------------------------------------
  
  static void processDMXChannels() {
    if (!dmxEnabled || !dmxConnected) {
      return;
    }
    
    // Check if system is homed (required for DMX control)
    if (!StepperController::isHomed()) {
      static uint32_t lastHomingWarning = 0;
      if (millis() - lastHomingWarning > 5000) {
        lastHomingWarning = millis();
        Serial.println("[DMX] System not homed - DMX control disabled");
      }
      return;
    }
    
    // Get current channel values (already cached by updateChannelCache)
    uint8_t channels[5];
    memcpy(channels, channelCache, NUM_CHANNELS);
    
    // Detect mode with hysteresis
    DMXMode newMode = detectModeWithHysteresis(channels[CH_MODE]);
    
    // Handle mode transitions
    if (newMode != currentMode) {
      Serial.printf("[DMX] Mode change: %s -> %s\n",
        currentMode == DMXMode::STOP ? "STOP" : (currentMode == DMXMode::CONTROL ? "CONTROL" : "HOME"),
        newMode == DMXMode::STOP ? "STOP" : (newMode == DMXMode::CONTROL ? "CONTROL" : "HOME"));
      
      lastMode = currentMode;
      currentMode = newMode;
      
      // Handle mode-specific transitions
      switch (newMode) {
        case DMXMode::STOP:
          // Stop motion
          {
            MotionCommand stopCmd;
            stopCmd.type = CommandType::STOP;
            stopCmd.timestamp = millis();
            stopCmd.commandId = 0;
            xQueueSend(g_motionCommandQueue, &stopCmd, 0);
          }
          break;
          
        case DMXMode::HOME:
          // Initiate homing
          {
            MotionCommand homeCmd;
            homeCmd.type = CommandType::HOME;
            homeCmd.timestamp = millis();
            homeCmd.commandId = 0;
            xQueueSend(g_motionCommandQueue, &homeCmd, 0);
          }
          break;
          
        case DMXMode::CONTROL:
          // Will start processing position commands
          break;
      }
    }
    
    // Process position control in CONTROL mode
    if (currentMode == DMXMode::CONTROL) {
      // Calculate position from DMX values
      float positionPercent;
      if (use16BitPosition) {
        // 16-bit mode: combine MSB and LSB
        uint16_t pos16 = (static_cast<uint16_t>(channels[CH_POSITION_MSB]) << 8) | channels[CH_POSITION_LSB];
        positionPercent = (pos16 / 65535.0f) * 100.0f;
      } else {
        // 8-bit mode: use MSB only
        positionPercent = (channels[CH_POSITION_MSB] / 255.0f) * 100.0f;
      }
      
      // Get position limits from StepperController
      int32_t minPos, maxPos;
      if (!StepperController::getPositionLimits(minPos, maxPos)) {
        return;  // Can't get limits
      }
      
      // Calculate actual target position
      int32_t range = maxPos - minPos;
      int32_t targetPosition = minPos + (int32_t)((range * positionPercent) / 100.0f);
      
      // Only send command if position changed significantly (prevent command flooding)
      if (lastTargetPosition == -1 || abs(targetPosition - lastTargetPosition) > 2) {
        // Get current configuration for max values
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (!config) return;
        
        // Calculate actual speed and acceleration from DMX values
        float speedPercent = (channels[CH_SPEED] / 255.0f) * 100.0f;
        float accelPercent = (channels[CH_ACCELERATION] / 255.0f) * 100.0f;
        
        float actualSpeed = (config->defaultProfile.maxSpeed * speedPercent) / 100.0f;
        float actualAccel = (config->defaultProfile.acceleration * accelPercent) / 100.0f;
        
        // Ensure minimum values for safety
        if (actualSpeed < 10.0f) actualSpeed = 10.0f;
        if (actualAccel < 10.0f) actualAccel = 10.0f;
        
        // Create motion command
        MotionCommand cmd;
        cmd.type = CommandType::MOVE_ABSOLUTE;
        cmd.profile = config->defaultProfile;  // Start with current profile
        cmd.profile.targetPosition = targetPosition;
        cmd.profile.maxSpeed = actualSpeed;
        cmd.profile.acceleration = actualAccel;
        cmd.profile.deceleration = actualAccel;  // Same as acceleration
        cmd.timestamp = millis();
        cmd.commandId = 0;
        
        // Send command to StepperController (non-blocking)
        if (xQueueSend(g_motionCommandQueue, &cmd, 0) == pdTRUE) {
          lastTargetPosition = targetPosition;
          
          // Log periodically to avoid spam
          static uint32_t lastLogTime = 0;
          if (millis() - lastLogTime > 1000) {
            lastLogTime = millis();
            Serial.printf("[DMX] Motion: Pos=%d (%.1f%%), Spd=%.0f, Acc=%.0f\n",
              targetPosition, positionPercent, actualSpeed, actualAccel);
          }
        }
      }
    }
  }
  
  // ----------------------------------------------------------------------------
  // Private Helper Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Update channel cache from DMX data
   */
  static void updateChannelCache() {
    // Ensure base channel + our channels don't exceed 512
    if (baseChannel + NUM_CHANNELS - 1 > 512) {
      return;
    }
    
    // Use the efficient readChannels method to read our 5 channels at once
    uint16_t channelsRead = dmx.readChannels(channelCache, baseChannel, NUM_CHANNELS);
    
    // Check if we got all channels
    if (channelsRead != NUM_CHANNELS) {
      // Partial read - might indicate a short DMX universe
      for (uint16_t i = channelsRead; i < NUM_CHANNELS; i++) {
        channelCache[i] = 0;  // Clear unread channels
      }
    }
  }
  
  /**
   * Check for signal timeout
   */
  static void checkSignalTimeout() {
    if (dmxConnected && (millis() - lastPacketTime > signalTimeout)) {
      dmxConnected = false;
      currentState = DMXState::TIMEOUT;
      Serial.println("[DMX] Signal timeout");
    }
  }
  
  /**
   * DMX task running on Core 0
   */
  static void dmxTask(void* parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);  // 10ms update rate
    
    Serial.println("[DMX] Task started on Core 0");
    uint32_t loopCount = 0;
    
    while (true) {
      loopCount++;
      
      // Debug output every 10 seconds
      if (loopCount % 1000 == 0) {
        Serial.printf("[DMX] Task alive - Connected: %s, Mode: %s\n", 
          dmx.isConnected() ? "YES" : "NO",
          currentMode == DMXMode::STOP ? "STOP" : (currentMode == DMXMode::CONTROL ? "CONTROL" : "HOME"));
      }
      
      // Check if DMX is connected
      if (dmx.isConnected()) {
        // Check if we have new packets
        uint32_t currentPacketCount = dmx.getPacketCount();
        if (currentPacketCount > lastPacketCount) {
          // New packet(s) received
          dmxConnected = true;
          currentState = DMXState::SIGNAL_PRESENT;
          lastPacketTime = millis();
          totalPackets = currentPacketCount;
          errorPackets = dmx.getErrorCount();
          lastPacketCount = currentPacketCount;
          
          // Update our channel cache
          updateChannelCache();
          
          // Update system status
          SAFE_WRITE_STATUS(dmxState, DMXState::SIGNAL_PRESENT);
          SAFE_WRITE_STATUS(lastDMXUpdate, lastPacketTime);
        }
      } else {
        // No DMX signal
        if (dmxConnected) {
          dmxConnected = false;
          currentState = DMXState::NO_SIGNAL;
          Serial.println("[DMX] Signal lost");
        }
      }
      
      // Check for timeout (redundant with library's isConnected, but allows custom timeout)
      checkSignalTimeout();
      
      // Process DMX channels and generate motion commands
      processDMXChannels();
      
      // Wait for next cycle
      vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
  }
  
  // ----------------------------------------------------------------------------
  // Public Interface Functions
  // ----------------------------------------------------------------------------
  
  bool initialize() {
    if (moduleInitialized) {
      return true;  // Already initialized
    }
    
    Serial.println("[DMX] Initializing DMXReceiver with ESP32S3DMX...");
    
    // Initialize DMX on UART2 with RX pin from HardwareConfig
    // Note: ESP32S3DMX begin() parameters are: uart_num, rx_pin, tx_pin, enable_pin
    // For receive-only, we need to properly configure the pins
    Serial.printf("[DMX] Configuring UART2 with RX on GPIO %d\n", DMX_RO_PIN);
    
    // The ESP32S3DMX library might need the enable pin for RS485 direction control
    // Try with the DE/RE pin defined in HardwareConfig
    dmx.begin(2, DMX_RO_PIN, DMX_DI_PIN, DMX_DE_RE_PIN);  // UART2, GPIO 6 (RX), GPIO 4 (TX), GPIO 5 (DE/RE)
    
    // Make sure the RS485 transceiver is in receive mode
    pinMode(DMX_DE_RE_PIN, OUTPUT);
    digitalWrite(DMX_DE_RE_PIN, LOW);  // LOW = receive mode for most RS485 transceivers
    
    Serial.println("[DMX] RS485 transceiver set to receive mode");
    
    // Set initial state
    currentState = DMXState::NO_SIGNAL;
    dmxConnected = false;
    lastPacketTime = millis();
    lastPacketCount = 0;
    
    // Load base channel from config
    SAFE_READ_CONFIG(dmxStartChannel, baseChannel);
    if (baseChannel < 1 || baseChannel > 508) {  // 508 because we need 5 channels
      baseChannel = 1;  // Default to channel 1
    }
    
    // Load timeout from config
    SAFE_READ_CONFIG(dmxTimeout, signalTimeout);
    
    // Create DMX task on Core 0
    xTaskCreatePinnedToCore(
      dmxTask,            // Task function
      "DMXReceiver",      // Task name
      4096,               // Stack size
      NULL,               // Parameters
      1,                  // Priority (lower than StepperController)
      &dmxTaskHandle,     // Task handle
      0                   // Core 0 - Real-time operations
    );
    
    // Update system status
    SAFE_WRITE_STATUS(dmxState, DMXState::NO_SIGNAL);
    
    moduleInitialized = true;
    Serial.println("[DMX] DMXReceiver initialized successfully");
    Serial.print("[DMX] Base channel: ");
    Serial.println(baseChannel);
    Serial.print("[DMX] Timeout: ");
    Serial.print(signalTimeout);
    Serial.println(" ms");
    Serial.println("[DMX] Motion control integration active");
    
    return true;
  }
  
  bool update() {
    // Main update is handled by the Core 0 task
    // This function can be used for any Core 1 operations if needed
    return true;
  }
  
  bool isSignalPresent() {
    return dmxConnected && (currentState == DMXState::SIGNAL_PRESENT);
  }
  
  uint16_t getChannelValue(uint16_t channel) {
    // Validate channel number
    if (channel < 1 || channel > 512) {
      return 0;
    }
    
    // Use the library's read method for individual channel access
    return dmx.read(channel);
  }
  
  uint32_t getLastUpdateTime() {
    return lastPacketTime;
  }
  
  DMXState getState() {
    return currentState;
  }
  
  bool getUniverseData(uint8_t* buffer) {
    if (!buffer) {
      return false;
    }
    
    // Get direct access to DMX buffer and copy it
    const uint8_t* dmxBuffer = dmx.getBuffer();
    if (dmxBuffer) {
      // Copy channels 1-512 (skip start code at index 0)
      memcpy(buffer, dmxBuffer + 1, 512);
      return true;
    }
    
    return false;
  }
  
  bool setTimeout(uint32_t timeoutMs) {
    if (timeoutMs < 100 || timeoutMs > 60000) {
      return false;  // Invalid timeout
    }
    
    signalTimeout = timeoutMs;
    
    // Update config
    SAFE_WRITE_CONFIG(dmxTimeout, timeoutMs);
    
    return true;
  }
  
  bool getPacketStats(uint32_t& totalPkts, uint32_t& errorPkts) {
    totalPkts = totalPackets;
    errorPkts = errorPackets;
    return true;
  }
  
  bool resetStats() {
    // We can't reset the library's internal counters, but we can track our own
    totalPackets = dmx.getPacketCount();
    errorPackets = dmx.getErrorCount();
    lastPacketCount = totalPackets;
    return true;
  }
  
  // ----------------------------------------------------------------------------
  // Additional Public Functions for 5-Channel Implementation
  // ----------------------------------------------------------------------------
  
  /**
   * Get cached values for our 5 channels
   * @param cache Array of 5 bytes to receive channel values
   * @return true if values retrieved successfully
   */
  bool getChannelCache(uint8_t cache[5]) {
    memcpy(cache, channelCache, NUM_CHANNELS);
    return true;
  }
  
  /**
   * Set base channel for 5-channel operation
   * @param channel Base channel (1-508)
   * @return true if channel set successfully
   */
  bool setBaseChannel(uint16_t channel) {
    // Validate that base + 4 doesn't exceed 512
    if (channel < 1 || channel > 508) {
      return false;
    }
    
    baseChannel = channel;
    
    // Update config
    SAFE_WRITE_CONFIG(dmxStartChannel, channel);
    
    Serial.print("[DMX] Base channel set to: ");
    Serial.println(baseChannel);
    
    return true;
  }
  
  /**
   * Get current base channel
   * @return Current base channel (1-508)
   */
  uint16_t getBaseChannel() {
    return baseChannel;
  }
  
  /**
   * Get formatted channel values for display
   * @param buffer Buffer for formatted string
   * @param bufferSize Size of buffer
   * @return Number of characters written
   */
  size_t getFormattedChannelValues(char* buffer, size_t bufferSize) {
    return snprintf(buffer, bufferSize, 
      "Ch%d-Ch%d: [%3d,%3d,%3d,%3d,%3d]",
      baseChannel, baseChannel + 4,
      channelCache[0], channelCache[1], channelCache[2], 
      channelCache[3], channelCache[4]
    );
  }
  
  /**
   * Enable or disable DMX control
   * @param enable true to enable DMX control, false to disable
   */
  bool setDMXEnabled(bool enable) {
    dmxEnabled = enable;
    Serial.printf("[DMX] Control %s\n", enable ? "enabled" : "disabled");
    
    if (!enable && currentMode == DMXMode::CONTROL) {
      // Stop motion if disabling while in control mode
      MotionCommand stopCmd;
      stopCmd.type = CommandType::STOP;
      stopCmd.timestamp = millis();
      stopCmd.commandId = 0;
      xQueueSend(g_motionCommandQueue, &stopCmd, 0);
      currentMode = DMXMode::STOP;
    }
    
    return true;
  }
  
  /**
   * Check if DMX control is enabled
   * @return true if DMX control is enabled
   */
  bool isDMXEnabled() {
    return dmxEnabled;
  }
  
  /**
   * Set 16-bit position mode
   * @param enable true for 16-bit mode, false for 8-bit mode
   */
  bool set16BitMode(bool enable) {
    use16BitPosition = enable;
    Serial.printf("[DMX] Position mode: %s\n", enable ? "16-bit" : "8-bit");
    return true;
  }
  
  /**
   * Get current 16-bit mode setting
   * @return true if 16-bit mode is enabled
   */
  bool is16BitMode() {
    return use16BitPosition;
  }
  
  /**
   * Get current DMX mode
   * @return Current mode (STOP, CONTROL, HOME)
   */
  DMXMode getCurrentMode() {
    return currentMode;
  }
}
