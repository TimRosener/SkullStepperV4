# Changelog

All notable changes to the SkullStepperV4 project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [4.1.14] - 2025-02-08

### Fixed
- **Critical: Limit Safety Margin calculation** - Fixed incorrect application of safety margin
  - Removed 80% backoff factor; now uses exact configured margin value
  - Fixed coordinate system reset to properly set left limit at position 0
  - Corrected right limit calculation to use full margin from detected position
- **Web Interface Configuration Display** - Fixed missing limit safety margin value
  - Added limitSafetyMargin to status JSON updates
  - Removed duplicate HTML elements with same IDs in motion parameters
  - Consolidated motion parameters into single clean section
- **Position Tracking** - Improved limit detection and reporting
  - Added tracking of actual detected limit positions
  - Separated detected physical limits from operating limits
  - Enhanced debug output showing physical vs operating ranges

### Changed
- **Limit Safety Margin Range** - Minimum value changed from 50 to 0 steps
  - Allows operation right at limit switches if desired
  - Updated validation, web interface, and documentation
- **Web Interface Motion Parameters** - Simplified configuration display
  - Removed duplicate "Key" vs "All" parameter sections
  - Single unified motion parameters section
  - Fixed event listeners for all sliders

### Added
- New `getDetectedLimits()` function to retrieve actual switch positions
- Enhanced position reporting showing both physical and operating limits
- Clearer serial debug messages during homing sequence

## [4.1.13] - 2025-02-08

### Added
- System Diagnostics tab in web interface status section
- Real-time memory status display with percentage and color coding
- Task health monitoring showing running status for all system tasks
- System information display (CPU model, frequency, flash size, reset reason)
- Memory fragmentation tracking with largest allocatable block size
- Minimum free heap tracking since boot

### Changed
- Status section now has three tabs: System Status, DMX Status, and Diagnostics
- Memory percentage color changes based on available memory (red < 20%, yellow < 40%, green >= 40%)

## [4.1.12] - 2025-02-08

### Fixed
- Web interface status tabs not displaying due to JavaScript error
- `showStatusTab()` function was using undefined `event` parameter
- Status tab buttons now properly pass event parameter to handler
- Added safety check for event.target to prevent errors

## [4.1.11] - 2025-02-08

### Added
- Real-time DMX status display in web interface showing all 5 channels
- Calculated values display for DMX channels:
  - Position: Shows target position in steps
  - Speed: Shows speed in steps/s
  - Acceleration: Shows acceleration in steps/s²
  - Mode: Shows decoded mode (STOP/CONTROL/HOME) with color coding
- Tabbed interface for status panel (System Status / DMX Status tabs)

### Changed
- DMX status now appears in dedicated tab for cleaner interface
- Mode display uses color coding: STOP (red), CONTROL (green), HOME (orange)
- Simplified DMX configuration by removing unused dmxScale and dmxOffset parameters
- "DMX Offset" label changed to "DMX Start Ch" for clarity

### Fixed
- DMX start channel now correctly displays the configured channel number
- Removed non-functional DMX scale factor from configuration
- Cleaned up DMX position calculation to match actual system behavior

### Removed
- DMX Scale Factor configuration (was not implemented in motion control)
- DMX Offset configuration for position (system uses percentage-based mapping)

## [4.1.10] - 2025-02-07

### Fixed
- DMX zero value behavior - channels now properly scale from minimum to maximum instead of using defaults
- Serial output corruption by removing "Task alive" messages
- DMX speed/acceleration control now works correctly (0=slow, 255=fast)

### Added
- DMX_DEBUG_ENABLED flag to control debug output
- Better channel state monitoring (stuck LSB detection, channel wake events)

### Changed
- DMX speed/acceleration scaling now uses proportional control
- Debug output can be easily enabled/disabled for cleaner operation

## [4.1.10] - 2025-02-03

### Fixed
- DMX channel cache corruption when no new packets arrive
- System now only updates channel cache when receiving valid new DMX packets
- DMX speed/acceleration channels use system defaults when value is 0
- Removed repetitive "Move to" debug messages from StepperController

### Added
- Enhanced DMX debug output showing all 5 channels including mode
- Detection for DMX channel values suddenly dropping to zero
- Warning messages when using default speed/acceleration values
- Consolidated "Task alive" messages with regular debug output
- Channel wake detection showing which channels recover from 0
- LSB stuck detection for 16-bit position mode issues

### Changed
- DMX debug output now shows values every 1 second regardless of movement
- Improved debug message formatting for easier reading
- Task alive messages integrated into position update stream

## [4.1.9] - 2025-02-03

### Fixed
- Improved DMX idle handling - now always updates channel cache when connected
- Added position mismatch detection to force updates when DMX target differs from actual position
- Enhanced debug output to show channel values in periodic status messages

### Added
- Position comparison check between current position and DMX target
- Debug messages for position timeout and mismatch scenarios
- Channel value display in periodic debug output

## [4.1.8] - 2025-02-03

### Fixed
- DMX control becoming unresponsive after idle periods
- Added position tracking timeout to force updates after 30 seconds of inactivity
- Improved DMX connection monitoring with better debug output
- DMX channels are now always processed when connected, even without new packets
- Added periodic debug logging in CONTROL mode to diagnose issues

### Changed
- DMX task now shows time since last channel update in debug output
- More robust handling of DMX signal presence detection

## [4.1.7] - 2025-02-02

### Fixed
- Homing sequence now consistently uses the configured `homingSpeed` parameter throughout
- Previously, the final movement to home position would incorrectly use `maxSpeed`
- This ensures predictable and safe homing behavior at the user-configured speed

## [4.1.6] - 2025-02-02

### Changed
- DMX behavior when homing is required (on boot or after E-STOP)
- DMX position control is now disabled when system requires homing
- DMX HOME command (mode channel 170-255) can still be sent to initiate homing
- This allows DMX to trigger homing even when movement is otherwise blocked
- **DMX input is completely ignored during homing sequence**
- **Homing triggered by DMX only requires momentary signal (can return to STOP)**
- **Prevents homing loops and ensures homing completes without interruption**

### Added
- **Dynamic DMX speed/acceleration control during motion**
- Speed and acceleration changes from DMX now take effect immediately
- Updates are applied even while motor is moving to a position
- Small threshold (±2) prevents jitter from DMX signal noise
- Clear console logging shows when speed or acceleration changes

## [4.1.5] - 2025-02-02

### Fixed
- Auto-Home checkboxes now always visible in web interface
- Moved "Auto-Home on Boot" and "Auto-Home on E-Stop" options to new "Homing Options" section
- Checkboxes no longer hidden when system is not homed

## [4.1.4] - 2025-02-02

### Changed
- Web interface now shows single consolidated homing warning message
- Configuration parameters can now be changed without homing (only movement is restricted)
- Reorganized configuration tabs: removed Advanced tab, redistributed settings logically
- Motion-related advanced settings (jerk, emergency deceleration) moved to Motion & Limits tab
- DMX timeout setting moved to DMX tab with other DMX parameters

### Improved
- Cleaner web interface with reduced redundancy
- More logical organization of configuration parameters
- Better user experience with clear messaging about homing requirements

## [4.1.3] - 2025-02-02

### Fixed
- DMX speed and acceleration channels now properly control motion parameters
- StepperController applies speed/acceleration from motion commands instead of ignoring them
- Motion commands from DMX now use dynamic speed/acceleration values from channels 2 and 3

## [4.1.2] - 2025-02-02

### Added
- DMX Phase 1 implementation: Core infrastructure with ESP32S3DMX library
- 5-channel DMX operation with configurable base offset
- DMX signal monitoring and timeout detection
- Real-time DMX processing on Core 0

### Changed
- Improved DMX channel layout: Position LSB moved to channel 1

## [4.1.1] - 2025-02-02

### Added
- Home on Boot checkbox option for automatic initialization
- Home on E-Stop checkbox option for automatic recovery after emergency stop

### Fixed
- Auto-home on E-stop now properly initiates homing sequence using processMotionCommand()
- Corrected mutex handling in auto-home implementation
- Auto-home now triggers correctly when homing state is COMPLETE (not just IDLE)
- Limit fault is cleared before attempting auto-home to prevent command rejection

## [4.1.0] - 2025-02-02

### Added
- Percentage-based home position configuration (0-100% of detected range)
- MOVEHOME command to move to configured home position
- "Move to Home" button in web interface
- Continuous stress test in web interface (matches serial TEST command)
- Enhanced position limits tab with validation
- PUBLISHING.md documentation for project management standards
- CHANGELOG.md for version tracking

### Changed
- Home position now adapts to any mechanical installation
- Web interface TEST button now runs continuously until stopped
- Web interface TEST2 button renamed to "RANDOM MOVES" for clarity
- Position limits configuration requires homing first
- Improved tooltips and disabled states in web interface

### Fixed
- Web interface stress test now properly continues until stopped
- Position validation against hardware limits
- Configuration persistence for home position percentage

## [4.0.0] - 2025-01-31

### Added
- Complete web interface with WiFi access point
- Real-time WebSocket updates at 10Hz
- Web-based motion control and configuration
- System information display panel
- User-configurable homing speed parameter
- Enhanced JSON command processing

### Changed
- Motion parameters now properly load from flash on boot
- Improved limit switch detection with continuous monitoring
- Enhanced debouncing with 3-sample validation

### Fixed
- Configuration loading on system initialization
- Intermittent limit switch detection issues
- JSON command parsing for configuration

## [3.0.0] - 2025-01-30

### Added
- ODStepper/FastAccelStepper integration
- Auto-range homing sequence
- Hardware limit switch support
- PARAMS command for parameter listing
- TEST command for automated testing
- TEST2/RANDOMTEST command for random positioning

### Changed
- Complete motion control rewrite for smooth acceleration
- Motor now holds position by default
- Extended homing timeout to 90 seconds

### Fixed
- Critical limit switch bug during motion
- Motion jerking issues (mechanical - loose coupler)

## [2.0.0] - 2025-01-28

### Added
- Thread-safe global infrastructure
- FreeRTOS dual-core architecture
- Motion command queue system
- Complete serial interface with JSON API
- Configuration persistence with ESP32 Preferences

### Changed
- Modular architecture with complete isolation
- Separated real-time (Core 0) and communication (Core 1) tasks

## [1.0.0] - 2025-01-23

### Added
- Initial project structure
- Hardware configuration for ESP32-S3
- Basic module headers and interfaces
- Pin assignments for CL57Y stepper driver
- DMX interface definitions
- Limit switch configurations

---

## Version Numbering

This project uses semantic versioning:
- **Major version (X.0.0)**: Breaking changes or major architectural updates
- **Minor version (x.X.0)**: New features or significant enhancements
- **Patch version (x.x.X)**: Bug fixes and minor improvements

## Release Process

1. Update version in all required files (see PUBLISHING.md)
2. Update this CHANGELOG.md
3. Create git tag: `git tag -a v4.1.0 -m "Description"`
4. Push tag: `git push origin v4.1.0`
