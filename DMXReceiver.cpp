// ============================================================================
// File: DMXReceiver.cpp
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.1.13
// Date: 2025-02-03
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
#include <esp_task_wdt.h>  // For watchdog timer
#include <driver/gpio.h>    // For GPIO pull resistor configuration

// ============================================================================
// DMXReceiver Module - Phase 6 Implementation with ESP32S3DMX
// ============================================================================

namespace DMXReceiver {
  
  // ----------------------------------------------------------------------------
  // Debug control - set to false to disable debug output
  // ----------------------------------------------------------------------------
  static const bool DMX_DEBUG_ENABLED = true;  // Set to true only when debugging
  
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
  
  // Task health monitoring
  static uint32_t g_lastTaskUpdate = 0;
  static const uint32_t TASK_HEALTH_TIMEOUT_MS = 5000;  // Task is unhealthy if no update for 5 seconds
  
  // Mutex for channel cache protection
  static SemaphoreHandle_t channelCacheMutex = NULL;
  
  // Data validation
  static uint8_t consecutiveHomeReads = 0;  // Count consecutive 255 values on mode channel
  static const uint8_t HOME_TRIGGER_COUNT = 3;  // Require 3 consecutive reads of 255 to trigger homing
  static uint8_t lastValidChannels[NUM_CHANNELS] = {0};  // Last known good values
  
  // DMX control state
  static DMXMode currentMode = DMXMode::STOP;
  static DMXMode lastMode = DMXMode::STOP;
  static const uint8_t MODE_HYSTERESIS = 5;  // Prevent mode flickering
  static bool homingInProgress = false;  // Track if homing is active
  static bool homingTriggeredByDMX = false;  // Track if DMX initiated the homing
  static uint32_t lastChannelUpdateTime = 0;  // Track when channels were last updated
  
  // DMX configuration
  static bool dmxEnabled = true;  // DMX control enabled by default
  static bool use16BitPosition = false;  // Default to 8-bit mode
  static uint32_t lastMotionCommandTime = 0;
  static int32_t lastTargetPosition = -1;  // Track last commanded position
  static uint8_t lastSpeedValue = 0;  // Track last speed DMX value
  static uint8_t lastAccelValue = 0;  // Track last acceleration DMX value
  
  // ----------------------------------------------------------------------------
  // DMX Mode Detection with Hysteresis
  // ----------------------------------------------------------------------------
  
  static DMXMode detectModeWithHysteresis(uint8_t modeValue) {
    DMXMode detectedMode;
    
    // Determine raw mode from value
    // 1-100: STOP, 101-254: CONTROL, 255: HOME
    if (modeValue <= MODE_STOP_MAX) {
      detectedMode = DMXMode::STOP;
    } else if (modeValue <= MODE_CONTROL_MAX) {
      detectedMode = DMXMode::CONTROL;
    } else {
      detectedMode = DMXMode::HOME;  // Only at 255
    }
    
    // Apply hysteresis if mode is different from current
    if (detectedMode != currentMode) {
      // Check if we're far enough from the boundary
      switch (currentMode) {
        case DMXMode::STOP:
          // Need to get above 100+hysteresis to switch to CONTROL
          if (detectedMode == DMXMode::CONTROL && modeValue < (MODE_STOP_MAX + MODE_HYSTERESIS)) {
            return currentMode;  // Stay in STOP mode
          }
          break;
          
        case DMXMode::CONTROL:
          // Need to drop below 100-hysteresis to switch to STOP
          if (detectedMode == DMXMode::STOP && modeValue > (MODE_STOP_MAX - MODE_HYSTERESIS)) {
            return currentMode;  // Stay in CONTROL mode
          }
          // HOME is only at 255, no hysteresis needed
          break;
          
        case DMXMode::HOME:
          // Once in HOME mode, need to drop below 255 to exit
          // No hysteresis since HOME is only at exactly 255
          break;
      }
    }
    
    return detectedMode;
  }
  
  // ----------------------------------------------------------------------------
  // Process DMX Channels and Generate Motion Commands
  // ----------------------------------------------------------------------------
  
  static void processDMXChannels() {
    if (!dmxEnabled) {
      return;
    }
    
    // Check connection status
    if (!dmxConnected) {
      static uint32_t lastDisconnectWarning = 0;
      if (millis() - lastDisconnectWarning > 5000) {
        lastDisconnectWarning = millis();
        Serial.printf("[DMX] Warning: DMX not connected - waiting for signal (timeout=%lums)\n", signalTimeout);
      }
      return;
    }
    
    // Check if homing is in progress
    bool currentlyHoming = StepperController::isHoming();
    
    // If homing just started, set our flag
    if (currentlyHoming && !homingInProgress) {
      homingInProgress = true;
      Serial.println("[DMX] Homing in progress - ignoring all DMX input");
    }
    
    // If homing just completed, clear our flag and resume DMX processing
    if (!currentlyHoming && homingInProgress) {
      homingInProgress = false;
      homingTriggeredByDMX = false;
      Serial.println("[DMX] Homing complete - resuming DMX processing");
      // Reset mode to STOP to prevent immediate re-triggering
      currentMode = DMXMode::STOP;
    }
    
    // IGNORE ALL DMX INPUT WHILE HOMING
    if (homingInProgress) {
      return;  // Don't process any DMX channels during homing
    }
    
    // Get current channel values with mutex protection
    uint8_t channels[5] = {0};
    if (xSemaphoreTake(channelCacheMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      memcpy(channels, channelCache, NUM_CHANNELS);
      xSemaphoreGive(channelCacheMutex);
    } else {
      // Failed to get mutex, use last known values
      return;
    }
    
    // Detect mode with hysteresis
    DMXMode newMode = detectModeWithHysteresis(channels[CH_MODE]);
    
    // Check if system needs homing
    bool homingRequired = !StepperController::isHomed() || StepperController::isLimitFaultActive();
    
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
          // Only trigger homing if not already homing
          if (!currentlyHoming) {
            // Always allow homing command, even if system requires homing
            MotionCommand homeCmd;
            homeCmd.type = CommandType::HOME;
            homeCmd.timestamp = millis();
            homeCmd.commandId = 0;
            if (xQueueSend(g_motionCommandQueue, &homeCmd, 0) == pdTRUE) {
              homingTriggeredByDMX = true;
              Serial.println("[DMX] Homing command sent - DMX input will be ignored until complete");
            }
          }
          break;
          
        case DMXMode::CONTROL:
          // Check if system allows movement
          if (homingRequired) {
            Serial.println("[DMX] CONTROL mode blocked - homing required");
            // Optionally send a stop command to ensure no movement
            MotionCommand stopCmd;
            stopCmd.type = CommandType::STOP;
            stopCmd.timestamp = millis();
            stopCmd.commandId = 0;
            xQueueSend(g_motionCommandQueue, &stopCmd, 0);
          }
          break;
      }
    }
    
    // Process position control in CONTROL mode only if system is homed
    if (currentMode == DMXMode::CONTROL && !homingRequired) {
      
      // Calculate position from DMX values
      float positionPercent;
      if (use16BitPosition) {
        // 16-bit mode: combine MSB and LSB
        // Check for stuck LSB (DMX signal issue)
        static uint8_t lastLSB = 0;
        static uint32_t lsbStuckCount = 0;
        
        if (channels[CH_POSITION_MSB] > 0 && channels[CH_POSITION_LSB] == 0 && lastLSB > 0) {
          // LSB might be stuck at 0
          lsbStuckCount++;
          if (lsbStuckCount > 3) {
            // Silently switch to 8-bit mode temporarily
            positionPercent = (channels[CH_POSITION_MSB] / 255.0f) * 100.0f;
          } else {
            // Use last known good LSB value
            uint16_t pos16 = (static_cast<uint16_t>(channels[CH_POSITION_MSB]) << 8) | lastLSB;
            positionPercent = (pos16 / 65535.0f) * 100.0f;
          }
        } else {
          lsbStuckCount = 0;
          if (channels[CH_POSITION_LSB] > 0) {
            lastLSB = channels[CH_POSITION_LSB];
          }
          uint16_t pos16 = (static_cast<uint16_t>(channels[CH_POSITION_MSB]) << 8) | channels[CH_POSITION_LSB];
          positionPercent = (pos16 / 65535.0f) * 100.0f;
        }
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
      
      // Get current position to check if we need to send a command
      int32_t currentPos = StepperController::getCurrentPosition();
      bool isMoving = StepperController::isMoving();
      
      // Check if position, speed, or acceleration changed significantly
      bool positionChanged = (lastTargetPosition == -1 || abs(targetPosition - lastTargetPosition) > 2);
      bool speedChanged = (abs(channels[CH_SPEED] - lastSpeedValue) > 2);  // Small threshold to prevent jitter
      bool accelChanged = (abs(channels[CH_ACCELERATION] - lastAccelValue) > 2);
      
      // Check if we've been idle for too long (position tracking timeout)
      static uint32_t lastPositionUpdateTime = 0;
      bool positionTimeout = (millis() - lastPositionUpdateTime > 30000);  // 30 second timeout
      
      // Always check the actual DMX position value against current position
      // This ensures we detect changes even after idle periods
      bool atTargetPosition = (abs(currentPos - targetPosition) < 3);  // Within 3 steps of target
      
      // Force update if position changed OR if we've been idle too long OR DMX position differs from actual
      if (positionChanged || positionTimeout || !atTargetPosition) {
        lastPositionUpdateTime = millis();
        positionChanged = true;  // Force position update
        
        // Debug output when forcing update
        if (positionTimeout) {
          Serial.println("[DMX] Position timeout - forcing update");
        }
        if (!atTargetPosition && !positionChanged) {
          Serial.printf("[DMX] Position mismatch - Current: %d, DMX Target: %d\n", currentPos, targetPosition);
        }
      }
      
      // Only send speed/accel updates if we're moving or position is changing
      bool needsUpdate = positionChanged || (isMoving && (speedChanged || accelChanged));
      
      // Get current configuration for max values (needed for calculations)
      SystemConfig* config = SystemConfigMgr::getConfig();
      if (!config) return;
      
      // Calculate actual speed and acceleration from DMX values
      float actualSpeed, actualAccel;
      
      // Scale DMX values to actual speed (0 = minimum, 255 = maximum)
      // Use a minimum speed of 10 steps/sec to prevent stalling
      float speedPercent = (channels[CH_SPEED] / 255.0f) * 100.0f;
      actualSpeed = 10.0f + ((config->defaultProfile.maxSpeed - 10.0f) * speedPercent) / 100.0f;
      
      // Scale DMX values to actual acceleration (0 = minimum, 255 = maximum)
      // Use a minimum acceleration of 10 steps/sec² 
      float accelPercent = (channels[CH_ACCELERATION] / 255.0f) * 100.0f;
      actualAccel = 10.0f + ((config->defaultProfile.acceleration - 10.0f) * accelPercent) / 100.0f;
      
      // Debug output every 1 second showing all values
      static uint32_t lastDebugPrintTime = 0;
      static int32_t lastDebugPosition = -1;
      static float lastDebugSpeed = -1;
      static float lastDebugAccel = -1;
      static uint32_t lastTaskAliveTime = 0;
      
      if (DMX_DEBUG_ENABLED && millis() - lastDebugPrintTime >= 1000) {  // Every 1 second
        lastDebugPrintTime = millis();
        
        // Check what changed since last print
        bool posChanged = (lastDebugPosition != targetPosition);
        bool spdChanged = (lastDebugSpeed != actualSpeed);
        bool accChanged = (lastDebugAccel != actualAccel);
        
        // Check for anomalies
        bool lsbStuckAtZero = (channels[CH_POSITION_MSB] > 0 && channels[CH_POSITION_LSB] == 0);
        
        // Always print current values
        Serial.printf("[DMX] Pos: %d (%.1f%%) Spd: %.0f Acc: %.0f | Current: %d | Moving: %s",
                      targetPosition, positionPercent, actualSpeed, actualAccel,
                      currentPos, isMoving ? "YES" : "NO");
        
        // Add change indicators
        if (posChanged || spdChanged || accChanged) {
          Serial.print(" [Changed:");
          if (posChanged) Serial.print(" POS");
          if (spdChanged) Serial.print(" SPD");
          if (accChanged) Serial.print(" ACC");
          Serial.print("]");
        }
        
        // Add DMX channel raw values for reference
        Serial.printf(" | DMX[%d,%d,%d,%d,%d]", 
                      channels[CH_POSITION_MSB], channels[CH_POSITION_LSB],
                      channels[CH_SPEED], channels[CH_ACCELERATION],
                      channels[CH_MODE]);
        
        // Warn about anomalies
        if (lsbStuckAtZero) {
          Serial.print(" [LSB STUCK!]");
        }
        
        Serial.println();
        
        // Task alive status removed to clean up serial output
        
        // Update last values
        lastDebugPosition = targetPosition;
        lastDebugSpeed = actualSpeed;
        lastDebugAccel = actualAccel;
      }
      
      // Send command if any parameter changed and update is needed
      if (needsUpdate) {
        
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
          lastSpeedValue = channels[CH_SPEED];
          lastAccelValue = channels[CH_ACCELERATION];
          
          // Command sent - no need for additional logging here as debug output handles it
        }
      }
    } else if (currentMode == DMXMode::CONTROL && homingRequired) {
      // Periodically remind that homing is required
      static uint32_t lastHomingWarning = 0;
      if (millis() - lastHomingWarning > 5000) {
        lastHomingWarning = millis();
        Serial.println("[DMX] Position control blocked - system requires homing");
        Serial.println("[DMX] Set mode channel to 255 to initiate homing");
      }
    } else if (currentMode != DMXMode::CONTROL) {
      // Debug output for non-CONTROL modes
      static uint32_t lastModeDebugTime = 0;
      static uint32_t lastTaskAliveTime = 0;
      
      if (millis() - lastModeDebugTime >= 5000) {  // Every 5 seconds when not in control
        lastModeDebugTime = millis();
        Serial.printf("[DMX] Mode: %s | DMX Channels[%d,%d,%d,%d,%d] | Homing Required: %s\n",
                      currentMode == DMXMode::STOP ? "STOP" : 
                      (currentMode == DMXMode::HOME ? "HOME" : "UNKNOWN"),
                      channels[0], channels[1], channels[2], channels[3], channels[4],
                      homingRequired ? "YES" : "NO");
        
        // Task alive status removed to clean up serial output
      }
    }
    
    // Connection status tracking
    static uint32_t lastConnectionDebugTime = 0;
    if (millis() - lastConnectionDebugTime >= 10000) {  // Every 10 seconds
      lastConnectionDebugTime = millis();
      if (!dmxConnected) {
        Serial.println("[DMX] WARNING: No DMX signal detected");
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
    
    // Save previous values for comparison
    static uint8_t previousCache[NUM_CHANNELS] = {0};
    bool hadNonZeroValues = false;
    for (int i = 0; i < NUM_CHANNELS; i++) {
      if (previousCache[i] > 0) {
        hadNonZeroValues = true;
        break;
      }
    }
    
    // Create temporary buffer for reading
    uint8_t tempBuffer[NUM_CHANNELS];
    
    // Use the efficient readChannels method to read our 5 channels at once
    uint16_t channelsRead = dmx.readChannels(tempBuffer, baseChannel, NUM_CHANNELS);
    
    // Check if we got all channels
    if (channelsRead != NUM_CHANNELS) {
      // Partial read - might indicate a short DMX universe
      Serial.printf("[DMX] Warning: Only read %d of %d channels\n", channelsRead, NUM_CHANNELS);
      for (uint16_t i = channelsRead; i < NUM_CHANNELS; i++) {
        tempBuffer[i] = 0;  // Clear unread channels
      }
    }
    
    // Validate data before updating cache
    bool dataValid = true;
    
    // Check for suspicious patterns (all 255s, all 0s except one channel)
    int zeroCount = 0;
    int ffCount = 0;
    for (int i = 0; i < NUM_CHANNELS; i++) {
      if (tempBuffer[i] == 0) zeroCount++;
      if (tempBuffer[i] == 255) ffCount++;
    }
    
    // Suspicious if all channels are 255, or 4 channels are 0 and one is 255
    if (ffCount == NUM_CHANNELS || (zeroCount == NUM_CHANNELS - 1 && ffCount == 1)) {
      dataValid = false;
      Serial.printf("[DMX] Suspicious data pattern detected: zeros=%d, 255s=%d\n", zeroCount, ffCount);
    }
    
    // Special validation for mode channel (255 = homing)
    if (tempBuffer[CH_MODE] == 255) {
      // Check if this is a sudden spike
      if (previousCache[CH_MODE] < 250) {
        consecutiveHomeReads++;
        if (consecutiveHomeReads < HOME_TRIGGER_COUNT) {
          // Not enough consecutive reads, use previous value
          tempBuffer[CH_MODE] = previousCache[CH_MODE];
          Serial.printf("[DMX] Mode=255 detected, count=%d/%d, filtering...\n", 
                       consecutiveHomeReads, HOME_TRIGGER_COUNT);
        } else {
          Serial.println("[DMX] Mode=255 confirmed after multiple reads, allowing HOME trigger");
        }
      }
    } else {
      consecutiveHomeReads = 0;  // Reset counter
    }
    
    // Update cache with mutex protection
    if (xSemaphoreTake(channelCacheMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      if (dataValid) {
        // Check for significant changes before updating
        bool significantChange = false;
        for (int i = 0; i < NUM_CHANNELS; i++) {
          int diff = abs(tempBuffer[i] - channelCache[i]);
          if (diff > 5 && i != CH_MODE) {  // Allow small changes, except mode
            significantChange = true;
          }
          if (i == CH_MODE && tempBuffer[i] != channelCache[i]) {
            // Mode change is always significant
            significantChange = true;
            Serial.printf("[DMX] Mode channel changing: %d -> %d\n", channelCache[i], tempBuffer[i]);
          }
        }
        
        if (significantChange) {
          Serial.printf("[DMX] Channel update: [%d,%d,%d,%d,%d] -> [%d,%d,%d,%d,%d]\n",
                       channelCache[0], channelCache[1], channelCache[2], channelCache[3], channelCache[4],
                       tempBuffer[0], tempBuffer[1], tempBuffer[2], tempBuffer[3], tempBuffer[4]);
        }
        
        memcpy(channelCache, tempBuffer, NUM_CHANNELS);
        memcpy(lastValidChannels, tempBuffer, NUM_CHANNELS);
      } else {
        // Use last known good values
        Serial.println("[DMX] Invalid data detected, using last known good values");
        memcpy(channelCache, lastValidChannels, NUM_CHANNELS);
      }
      xSemaphoreGive(channelCacheMutex);
    }
    
    // Check if all values went to zero
    bool allZeros = true;
    for (int i = 0; i < NUM_CHANNELS; i++) {
      if (channelCache[i] != 0) {
        allZeros = false;
        break;
      }
    }
    
    // Debug if values suddenly went to all zeros
    if (allZeros && hadNonZeroValues) {
      Serial.println("[DMX] WARNING: All channel values suddenly went to 0!");
      Serial.printf("[DMX] Previous values were: [%d,%d,%d,%d,%d]\n",
                    previousCache[0], previousCache[1], previousCache[2], 
                    previousCache[3], previousCache[4]);
    }
    
    // Save current values for next comparison
    memcpy(previousCache, tempBuffer, NUM_CHANNELS);  // Use tempBuffer, not channelCache
  }
  
  /**
   * Check for signal timeout
   */
  static void checkSignalTimeout() {
    if (dmxConnected && (millis() - lastPacketTime > signalTimeout)) {
      dmxConnected = false;
      currentState = DMXState::TIMEOUT;
      Serial.printf("[DMX] Signal timeout - no packets for %lums (timeout=%lums)\n", 
                    millis() - lastPacketTime, signalTimeout);
    }
  }
  
  /**
   * DMX task running on Core 0
   */
  static void dmxTask(void* parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);  // 10ms update rate
    
    // Add this task to watchdog
    esp_task_wdt_add(NULL);
    
    Serial.println("[DMX] Task started on Core 0");
    Serial.println("[DMX] Watchdog timer active (10s timeout)");
    uint32_t loopCount = 0;
    uint32_t lastWdtFeed = 0;
    
    while (true) {
      // Update task health timestamp
      g_lastTaskUpdate = millis();
      loopCount++;
      
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
          lastChannelUpdateTime = millis();
          
          // Check if all channels went to 0 (force position update when values return)
          static bool allChannelsWereZero = false;
          bool allChannelsZero = true;
          for (int i = 0; i < NUM_CHANNELS; i++) {
            if (channelCache[i] != 0) {
              allChannelsZero = false;
              break;
            }
          }
          
          // Force position update when channels return from all-zero state
          if (!allChannelsZero && allChannelsWereZero) {
            lastTargetPosition = -1;  // Force position sync
          }
          allChannelsWereZero = allChannelsZero;
          
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
      
      // Always process DMX channels if we have a valid signal
      // Use the cached values from the last valid packet
      if (dmxConnected) {
        // Don't update cache here - only use what was cached from valid packets
        processDMXChannels();
      }
      
      // Feed watchdog timer periodically (every second)
      if (millis() - lastWdtFeed > 1000) {
        esp_task_wdt_reset();
        lastWdtFeed = millis();
      }
      
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
    
    // Create mutex for channel cache protection
    channelCacheMutex = xSemaphoreCreateMutex();
    if (channelCacheMutex == NULL) {
      Serial.println("[DMX] ERROR: Failed to create channel cache mutex");
      return false;
    }
    
    // Initialize DMX on UART2 with RX pin from HardwareConfig
    // Note: ESP32S3DMX begin() parameters are: uart_num, rx_pin, tx_pin, enable_pin
    // For receive-only, we need to properly configure the pins
    Serial.printf("[DMX] Configuring UART2 with RX on GPIO %d\n", DMX_RO_PIN);
    
    // Configure DE/RE pin with pull-down to ensure stable receive mode
    pinMode(DMX_DE_RE_PIN, OUTPUT);
    digitalWrite(DMX_DE_RE_PIN, LOW);  // LOW = receive mode for RS485 transceiver
    // Add internal pull-down to prevent floating
    gpio_set_pull_mode((gpio_num_t)DMX_DE_RE_PIN, GPIO_PULLDOWN_ONLY);
    
    // Initialize DMX library
    // Note: The RX/TX pins might be swapped based on the "SWAPPED" comment in HardwareConfig
    // Using: UART2, GPIO 6 (RX), GPIO 4 (TX), GPIO 5 (DE/RE)
    dmx.begin(2, DMX_RO_PIN, DMX_DI_PIN, DMX_DE_RE_PIN);
    
    // Double-check DE/RE is still low after library init
    digitalWrite(DMX_DE_RE_PIN, LOW);
    
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
    bool success = false;
    if (xSemaphoreTake(channelCacheMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      memcpy(cache, channelCache, NUM_CHANNELS);
      xSemaphoreGive(channelCacheMutex);
      success = true;
    }
    return success;
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
    uint8_t safeCache[NUM_CHANNELS] = {0};
    if (xSemaphoreTake(channelCacheMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      memcpy(safeCache, channelCache, NUM_CHANNELS);
      xSemaphoreGive(channelCacheMutex);
    }
    return snprintf(buffer, bufferSize, 
      "Ch%d-Ch%d: [%3d,%3d,%3d,%3d,%3d]",
      baseChannel, baseChannel + 4,
      safeCache[0], safeCache[1], safeCache[2], 
      safeCache[3], safeCache[4]
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
  
  /**
   * Check if the DMX task is healthy (responding within timeout)
   * @return true if task has updated within last 5 seconds
   */
  bool isTaskHealthy() {
    // Check if task has updated within timeout period
    uint32_t timeSinceUpdate = millis() - g_lastTaskUpdate;
    return (timeSinceUpdate < TASK_HEALTH_TIMEOUT_MS);
  }
  
  /**
   * Get the last task update timestamp
   * @return milliseconds timestamp of last task update
   */
  uint32_t getLastTaskUpdateTime() {
    return g_lastTaskUpdate;
  }
  
  /**
   * Get the DMX task handle for monitoring
   * @return TaskHandle_t or nullptr if not initialized
   */
  TaskHandle_t getTaskHandle() {
    return dmxTaskHandle;
  }
}
