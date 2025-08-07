# Changelog

All notable changes to the SkullStepperV4 project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
