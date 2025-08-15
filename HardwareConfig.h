// ============================================================================
// File: HardwareConfig.h
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.1.13
// Date: 2025-02-08
// Author: Tim Rosener
// Description: Hardware pin definitions and system constants for ESP32-S3
// License: MIT
// ============================================================================

#ifndef HARDWARECONFIG_H
#define HARDWARECONFIG_H

// ============================================================================
// ESP32-S3 Hardware Configuration for SkullStepperV4
// ============================================================================

// ----------------------------------------------------------------------------
// CL57Y Stepper Driver Pins
// ----------------------------------------------------------------------------
#define STEPPER_STEP_PIN        7    // GPIO 7 → CL57Y PP+ (Open-drain + 1.8kΩ to +5V)
#define STEPPER_DIR_PIN         15   // GPIO 15 → CL57Y DIR+ (Open-drain + 1.8kΩ to +5V)
#define STEPPER_ENABLE_PIN      16   // GPIO 16 → CL57Y MF+ (Open-drain + 1.8kΩ to +5V)
#define STEPPER_ALARM_PIN       8    // GPIO 8 ← CL57Y ALARM+ (Input with pull-up) ✓ CONFIRMED

// ----------------------------------------------------------------------------
// DMX Interface (MAX485) Pins
// ----------------------------------------------------------------------------
#define DMX_RO_PIN              6    // GPIO 6 (UART2 RX) - SWAPPED
#define DMX_DI_PIN              4    // GPIO 4 (UART2 TX) - SWAPPED
#define DMX_DE_RE_PIN           5    // GPIO 5 (Direction control - DE/RE tied together)

// ----------------------------------------------------------------------------
// Limit Switch Pins
// ----------------------------------------------------------------------------
#define LEFT_LIMIT_PIN          17   // GPIO 17 (Active low with pull-up) ✓ CONFIRMED
#define RIGHT_LIMIT_PIN         18   // GPIO 18 (Active low with pull-up) ✓ CONFIRMED

// ----------------------------------------------------------------------------
// Limit Switch Noise Filtering Recommendations
// ----------------------------------------------------------------------------
// To prevent false triggers from stepper motor EMI, add RC filters:
// 
// Limit Switch ----+----[1kΩ]----+---- GPIO Pin
//                  |              |
//                 GND          [0.1µF]
//                                |
//                               GND
//
// Additional recommendations:
// - Use shielded cable for limit switches (shield to GND at ESP32 end only)
// - Keep limit switch wires away from motor/power cables
// - Use twisted pair wiring if shielded cable unavailable
// - Consider ferrite beads on cables near ESP32
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Hardware Constants
// ----------------------------------------------------------------------------
#define STEPPER_STEPS_PER_REV   400  // Steps per revolution (1.8° stepper)
#define STEPPER_MICROSTEPS      1    // CL57Y microstep setting
#define TOTAL_STEPS_PER_REV     (STEPPER_STEPS_PER_REV * STEPPER_MICROSTEPS)

// DMX Configuration
#define DMX_UART_NUM            2    // Use UART2 for DMX
#define DMX_BAUD_RATE           250000
#define DMX_UNIVERSE_SIZE       512
#define DMX_START_CHANNEL       1    // DMX channel for position control

// RMT Configuration for Hardware Pulse Generation
#define STEPPER_RMT_CHANNEL     RMT_CHANNEL_0    // RMT channel for step pulses
#define RMT_CLK_DIV             80               // 80MHz / 80 = 1MHz (1μs resolution)
#define RMT_MEM_BLOCK_NUM       1                // Memory blocks for RMT
#define RMT_IDLE_LEVEL          RMT_IDLE_LEVEL_LOW // Idle level for step pin

// Timing Constants for 25% Duty Cycle Step Pulses
#define MIN_STEP_PULSE_WIDTH    2    // Minimum step pulse width (microseconds) - CL57Y requirement
#define STEP_PULSE_DUTY_CYCLE   0.25 // 25% duty cycle for step pulses
#define MIN_STEP_PERIOD         100  // Minimum step period (10kHz max frequency)
#define MAX_STEP_FREQUENCY      10000 // Maximum step frequency (Hz)
#define LIMIT_SWITCH_DEBOUNCE   100  // Limit switch debounce time (ms) - Increased for noise immunity

// Calculate pulse timing from frequency for RMT
#define RMT_PERIOD_FROM_FREQ(freq)      (1000000.0f / (freq))              // Period in microseconds
#define RMT_PULSE_WIDTH_FROM_PERIOD(period) ((uint32_t)((period) * 0.25f)) // 25% duty cycle
#define RMT_PULSE_GAP_FROM_PERIOD(period)   ((uint32_t)((period) * 0.75f)) // 75% low time

// Legacy constant for compatibility
#define MIN_STEP_INTERVAL       MIN_STEP_PERIOD  // Compatibility with existing code

// Motion Limits
#define MAX_POSITION_STEPS      (TOTAL_STEPS_PER_REV * 2)  // 2 full revolutions max
#define MIN_POSITION_STEPS      0
#define DEFAULT_MAX_SPEED       5000 // Steps per second
#define DEFAULT_ACCELERATION    5000  // Steps per second²

// Safety Constants
#define EMERGENCY_STOP_DECEL    2000 // Emergency deceleration (steps/sec²)
#define POSITION_TOLERANCE      0    // Position tolerance (steps)
#define ALARM_TIMEOUT_MS        100  // Stepper alarm timeout

// Serial Interface
#define SERIAL_BAUD_RATE        115200
#define SERIAL_TIMEOUT_MS       1000

// System Timing
#define MAIN_LOOP_INTERVAL_MS   10   // Main loop cycle time
#define STATUS_UPDATE_INTERVAL_MS 100 // Status reporting interval

#endif // HARDWARECONFIG_H