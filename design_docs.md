# SkullStepperV4 Design Document
## ESP32-S3 Modular Stepper Control System
### Version 4.1.0 - Production Ready
### Last Updated: 2025-02-02

## Executive Summary

SkullStepperV4 is a complete, production-ready stepper motor control system built on the ESP32-S3 platform. The system features industrial-grade motion control with automatic homing, position limits detection, DMX control integration, comprehensive safety monitoring, and both serial and web interfaces for control and configuration.

## System Architecture

### Core Design Principles
1. **Modular Architecture**: Separate modules for each major function
2. **Thread Safety**: Dual-core ESP32-S3 with proper mutex protection
3. **Real-time Performance**: FastAccelStepper library for smooth motion
4. **Multiple Interfaces**: Serial (USB/UART) and Web (HTTP/WebSocket)
5. **Persistent Configuration**: Flash-based storage that survives power cycles
6. **Comprehensive Safety**: Hardware limits, software bounds, emergency stop

### Hardware Platform
- **Microcontroller**: ESP32-S3-WROOM-1
- **Motion Control**: TMC2209 stepper driver
- **Position Feedback**: Optical limit switches
- **External Control**: DMX512 interface
- **User Interface**: Web server with real-time WebSocket updates

## Module Overview

### 1. GlobalInfrastructure (Core 0 & 1)
- **Purpose**: System-wide data structures and thread synchronization
- **Features**:
  - Mutex-protected shared data structures
  - Inter-module communication queues
  - Thread-safe status updates
  - System initialization and shutdown

### 2. StepperController (Core 0)
- **Purpose**: Real-time motion control and position management
- **Features**:
  - FastAccelStepper integration
  - Automatic homing with range detection
  - Position limit enforcement
  - Motion profiles with jerk limitation
  - Emergency stop capability
  - Configurable home position (percentage-based)

### 3. SafetyMonitor (Core 0)
- **Purpose**: Hardware monitoring and fault detection
- **Features**:
  - Limit switch monitoring
  - Stepper driver alarm detection
  - Emergency stop handling
  - Fault state management
  - Automatic recovery after homing

### 4. SystemConfig (Core 1)
- **Purpose**: Persistent configuration management
- **Features**:
  - Flash-based storage using ESP32 Preferences
  - Motion profile parameters
  - Position limits and home position percentage
  - DMX configuration
  - Safety settings
  - JSON import/export

### 5. SerialInterface (Core 1)
- **Purpose**: USB/UART command interface
- **Features**:
  - Human-readable command syntax
  - JSON API for automation
  - Real-time status updates
  - Comprehensive help system
  - Error reporting with context

### 6. WebInterface (Core 1)
- **Purpose**: Web-based control panel
- **Features**:
  - Responsive HTML5 interface
  - Real-time WebSocket updates
  - Motion control with position feedback
  - Configuration management
  - System diagnostics
  - Stress testing tools

### 7. DMXReceiver (Core 1)
- **Purpose**: DMX512 protocol integration
- **Features**:
  - 512 channel support
  - Position control via DMX
  - Configurable scaling and offset
  - Signal timeout detection
  - Status monitoring

## Key Features

### Motion Control
- **Automatic Homing**: Detects physical travel limits and sets safe operating bounds
- **Position Limits**: Hardware and software enforced boundaries
- **Motion Profiles**: Configurable speed, acceleration, and jerk
- **Home Position**: Configurable as percentage of range (0-100%)
- **Emergency Stop**: Immediate halt with high deceleration

### User Interfaces
- **Serial Commands**: Full control via USB or UART
- **Web Interface**: Browser-based control panel
- **DMX Control**: Integration with lighting consoles
- **Status Feedback**: Real-time position and state updates

### Safety Features
- **Limit Switches**: Hardware protection at travel extremes
- **Software Limits**: Configurable boundaries within hardware limits
- **Fault Detection**: Automatic stop on limit violations
- **Recovery**: Requires homing to clear faults
- **Emergency Stop**: Multiple ways to halt motion immediately

### Configuration
- **Persistent Storage**: All settings saved to flash
- **Runtime Updates**: Change parameters without restarting
- **Import/Export**: JSON configuration backup/restore
- **Validation**: Parameters checked before applying

## Recent Enhancements (2025-02-02)

### 1. Fixed Stress Test Implementation
- Web interface TEST button now runs continuously (matches serial behavior)
- Automatic movement between 10% and 90% of range
- Progress tracking and status updates
- Proper stop handling via STOP/E-STOP buttons

### 2. Enhanced Position Limits Configuration
- Limits tab requires homing before configuration
- Displays detected physical range
- Validates entries against hardware limits
- Clear visual feedback for system state

### 3. Configurable Home Position
- Percentage-based (0-100%) instead of fixed position
- Automatically scales to detected range
- Survives power cycles
- Configurable via web and serial interfaces

### 4. Move to Home Button
- One-click return to configured home position
- Available in web interface
- MOVEHOME command in serial interface
- Requires homing before use

## System States

### Motion States
- **IDLE**: Ready for commands
- **ACCELERATING**: Ramping up to target speed
- **CONSTANT_VELOCITY**: Moving at target speed
- **DECELERATING**: Slowing to stop
- **HOMING**: Running homing sequence
- **POSITION_HOLD**: Maintaining position

### Safety States
- **NORMAL**: No faults detected
- **LEFT_LIMIT_ACTIVE**: At left travel limit
- **RIGHT_LIMIT_ACTIVE**: At right travel limit
- **BOTH_LIMITS_ACTIVE**: Hardware fault condition
- **STEPPER_ALARM**: Driver fault detected
- **EMERGENCY_STOP**: E-stop activated

### System States
- **UNINITIALIZED**: Startup state
- **INITIALIZING**: Loading configuration
- **READY**: Normal operation
- **RUNNING**: Motion in progress
- **STOPPING**: Controlled stop
- **STOPPED**: Motion halted
- **ERROR**: Fault condition
- **EMERGENCY_STOP**: E-stop state

## Configuration Parameters

### Motion Parameters
- **maxSpeed**: 0-10000 steps/sec (default: 1000)
- **acceleration**: 0-20000 steps/sec² (default: 500)
- **jerk**: 0-50000 steps/sec³ (default: 1000)
- **homingSpeed**: 0-10000 steps/sec (default: 940)
- **homePositionPercent**: 0-100% (default: 50%)
- **emergencyDeceleration**: 100-50000 steps/sec² (default: 10000)

### Position Limits
- **minPosition**: Steps from home (auto-detected)
- **maxPosition**: Steps from home (auto-detected)
- **Position margin**: 25 steps safety buffer

### DMX Configuration
- **dmxStartChannel**: 1-512 (default: 1)
- **dmxScale**: Steps per DMX unit (default: 1.0)
- **dmxOffset**: Position offset in steps (default: 0)
- **dmxTimeout**: Signal loss timeout in ms (default: 5000)

## Testing Features

### Stress Test (TEST)
- Continuous movement between 10% and 90% of range
- Validates mechanical reliability
- Tracks cycle count
- Stops on limit faults

### Random Test (TEST2/RANDOMTEST)
- Moves to 10 random positions
- Tests positioning accuracy
- Validates full range operation
- Progress reporting

## Production Deployment

### Initial Setup
1. Install hardware (stepper, limits, driver)
2. Connect via USB or WiFi (AP mode)
3. Run HOME command to detect range
4. Configure parameters as needed
5. Save configuration to flash

### Operation
1. System starts with saved configuration
2. Home on power-up if required
3. Accept commands via serial/web/DMX
4. Monitor status and faults
5. Emergency stop always available

### Maintenance
- Export configuration for backup
- Monitor system diagnostics
- Check limit switch operation
- Verify position accuracy
- Update firmware via USB

## Future Enhancements

### Planned Features
- Multiple motion profiles
- Acceleration curves
- Position presets
- Sequence programming
- Data logging

### Possible Extensions
- Encoder feedback
- Multi-axis coordination
- Remote monitoring
- Cloud integration
- Mobile app

## Conclusion

SkullStepperV4 represents a complete, production-ready motion control solution. The modular architecture, comprehensive safety features, and multiple control interfaces make it suitable for professional installations requiring reliable, precise stepper motor control. The system's automatic homing, configurable parameters, and robust error handling ensure safe operation in demanding environments.
