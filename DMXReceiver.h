// ============================================================================
// File: DMXReceiver.h
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.0.0
// Date: 2025-01-23
// Author: Tim Rosener
// Description: DMXReceiver module interface - DMX512 signal reception
// License: MIT
// Phase: 1 (Stub Implementation) - Ready for Phase 6 development
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
  // Module Interface Functions (Phase 1: Stub implementations)
  // ----------------------------------------------------------------------------
  
  /**
   * Initialize the DMX receiver module
   * Sets up UART2, MAX485 interface, and DMX packet processing
   * @return true if initialization successful
   */
  bool initialize();
  
  /**
   * Update function called every millisecond from Core 0
   * Processes incoming DMX packets and validates timing
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
   * @param timeoutMs timeout in milliseconds
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
}

#endif // DMXRECEIVER_H