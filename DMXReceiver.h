// ============================================================================
// File: DMXReceiver.h
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.1.2
// Date: 2025-02-02
// Author: Tim Rosener
// Description: DMXReceiver module interface - DMX512 signal reception
// License: MIT
// Phase: 6 (Active Development) - ESP32S3DMX Library Integration
// ============================================================================

#ifndef DMXRECEIVER_H
#define DMXRECEIVER_H

#include "GlobalInterface.h"
#include "HardwareConfig.h"

// ============================================================================
// DMXReceiver Module - Core 0 Real-Time DMX Reception
// ============================================================================

namespace DMXReceiver {
  
  // ----------------------------------------------------------------------------
  // Module Constants
  // ----------------------------------------------------------------------------
  
  // DMX channel layout (base + offset)
  const uint8_t CH_POSITION_MSB = 0;  // Position coarse (8-bit mode)
  const uint8_t CH_POSITION_LSB = 1;  // Position fine (16-bit mode)
  const uint8_t CH_ACCELERATION = 2;  // Acceleration limit (% of max)
  const uint8_t CH_SPEED = 3;         // Speed limit (% of max)
  const uint8_t CH_MODE = 4;          // Mode control
  
  // Mode thresholds
  const uint8_t MODE_STOP_MAX = 84;       // 0-84: STOP mode
  const uint8_t MODE_CONTROL_MAX = 170;   // 85-170: DMX CONTROL mode
  // 171-255: FORCE HOME mode
  
  // ----------------------------------------------------------------------------
  // Module Enums
  // ----------------------------------------------------------------------------
  
  enum class DMXMode {
    STOP,      // Motor holds position, ignores position channel
    CONTROL,   // Follows position channel  
    HOME       // Initiates homing sequence
  };
  
  // ----------------------------------------------------------------------------
  // Module Interface Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Initialize the DMX receiver module
   * Sets up UART2, ESP32S3DMX library, and DMX packet processing
   * @return true if initialization successful
   */
  bool initialize();
  
  /**
   * Update function called from Core 1 (if needed)
   * Main processing happens in Core 0 task
   * @return true if update successful
   */
  bool update();
  
  /**
   * Check if DMX signal is currently present
   * @return true if valid DMX signal detected
   */
  bool isSignalPresent();
  
  /**
   * Get value from specific DMX channel
   * @param channel DMX channel number (1-512)
   * @return channel value (0-255), 0 if invalid channel
   */
  uint16_t getChannelValue(uint16_t channel);
  
  /**
   * Get timestamp of last DMX packet update
   * @return millisecond timestamp of last valid packet
   */
  uint32_t getLastUpdateTime();
  
  /**
   * Get current DMX receiver state
   * @return current state enum
   */
  DMXState getState();
  
  /**
   * Get DMX universe data
   * @param buffer buffer to copy universe data (512 bytes)
   * @return true if data copied successfully
   */
  bool getUniverseData(uint8_t* buffer);
  
  /**
   * Set DMX timeout value
   * @param timeoutMs timeout in milliseconds (100-60000)
   * @return true if timeout set successfully
   */
  bool setTimeout(uint32_t timeoutMs);
  
  /**
   * Get packet statistics
   * @param totalPackets returns total packets received
   * @param errorPackets returns packets with errors
   * @return true if statistics retrieved
   */
  bool getPacketStats(uint32_t& totalPackets, uint32_t& errorPackets);
  
  /**
   * Reset packet statistics
   * @return true if statistics reset
   */
  bool resetStats();
  
  // ----------------------------------------------------------------------------
  // 5-Channel Specific Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Get cached values for our 5 channels
   * @param cache Array of 5 bytes to receive channel values
   * @return true if values retrieved successfully
   */
  bool getChannelCache(uint8_t cache[5]);
  
  /**
   * Set base channel for 5-channel operation
   * @param channel Base channel (1-508, since we need 5 consecutive channels)
   * @return true if channel set successfully
   */
  bool setBaseChannel(uint16_t channel);
  
  /**
   * Get current base channel
   * @return Current base channel (1-508)
   */
  uint16_t getBaseChannel();
  
  /**
   * Get formatted channel values for display
   * @param buffer Buffer for formatted string
   * @param bufferSize Size of buffer
   * @return Number of characters written
   */
  size_t getFormattedChannelValues(char* buffer, size_t bufferSize);
  
  // ----------------------------------------------------------------------------
  // Helper Functions for Channel Processing
  // ----------------------------------------------------------------------------
  
  /**
   * Detect current mode from mode channel value
   * @param modeValue Raw channel value (0-255)
   * @return Detected mode
   */
  inline DMXMode detectMode(uint8_t modeValue) {
    if (modeValue <= MODE_STOP_MAX) return DMXMode::STOP;
    if (modeValue <= MODE_CONTROL_MAX) return DMXMode::CONTROL;
    return DMXMode::HOME;
  }
  
  /**
   * Calculate position percentage from channel values
   * @param msb Position MSB channel value
   * @param lsb Position LSB channel value (for 16-bit mode)
   * @param use16Bit Whether to use 16-bit precision
   * @return Position as percentage (0.0 - 100.0)
   */
  inline float calculatePosition(uint8_t msb, uint8_t lsb, bool use16Bit) {
    if (use16Bit) {
      uint16_t pos16 = (static_cast<uint16_t>(msb) << 8) | lsb;
      return (pos16 / 65535.0f) * 100.0f;
    } else {
      return (msb / 255.0f) * 100.0f;
    }
  }
  
  /**
   * Scale DMX value to percentage
   * @param value DMX channel value (0-255)
   * @return Percentage (0.0 - 100.0)
   */
  inline float dmxToPercent(uint8_t value) {
    return (value / 255.0f) * 100.0f;
  }
  
  // ----------------------------------------------------------------------------
  // DMX Control Functions
  // ----------------------------------------------------------------------------
  
  /**
   * Enable or disable DMX control
   * @param enable true to enable DMX control, false to disable
   */
  bool setDMXEnabled(bool enable);
  
  /**
   * Check if DMX control is enabled
   * @return true if DMX control is enabled
   */
  bool isDMXEnabled();
  
  /**
   * Set 16-bit position mode
   * @param enable true for 16-bit mode, false for 8-bit mode
   */
  bool set16BitMode(bool enable);
  
  /**
   * Get current 16-bit mode setting
   * @return true if 16-bit mode is enabled
   */
  bool is16BitMode();
  
  /**
   * Get current DMX mode
   * @return Current mode (STOP, CONTROL, HOME)
   */
  DMXMode getCurrentMode();
}

#endif // DMXRECEIVER_H
