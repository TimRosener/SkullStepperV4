# SkullStepperV4 Design Document
## ESP32-S3 Modular Stepper Control System
### Version 4.1.0 - Production Ready
### Last Updated: 2025-02-02

## Executive Summary

SkullStepperV4 is a complete, production-ready stepper motor control system built on the ESP32-S3 platform. The system features industrial-grade motion control with automatic homing, position limits detection, DMX control integration, comprehensive safety monitoring, and both serial and web interfaces for control and configuration.

## Current Project Status (February 2025)

### ✅ COMPLETED - Production Ready v4.1.0

All planned features have been successfully implemented and tested:

1. **Core Motion Control** - FastAccelStepper integration with smooth acceleration profiles
2. **Automatic Homing** - Range detection with configurable home position (percentage-based)
3. **Safety Systems** - Hardware limits, software bounds, emergency stop, fault recovery
4. **Dual Interface** - Serial commands and web control panel with WebSocket updates
5. **DMX Integration** - Full DMX512 control with scaling and offset
6. **Persistent Configuration** - Flash storage for all parameters
7. **Stress Testing** - Continuous and random position test modes
8. **Position Feedback** - Real-time status via serial and web interfaces

The system is now feature-complete and ready for production deployment.

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
- **Key Update**: Added `homePositionPercent` to replace fixed `homePosition`

### 2. StepperController (Core 0)
- **Purpose**: Real-time motion control and position management
- **Features**:
  - FastAccelStepper integration
  - Automatic homing with range detection
  - Position limit enforcement
  - Motion profiles with jerk limitation
  - Emergency stop capability
  - Configurable home position (percentage-based)
- **Key Update**: Home position now calculated as percentage of detected range

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
- **Key Update**: Stores `homePositionPercent` instead of absolute position

### 5. SerialInterface (Core 1)
- **Purpose**: USB/UART command interface
- **Features**:
  - Human-readable command syntax
  - JSON API for automation
  - Real-time status updates
  - Comprehensive help system
  - Error reporting with context
- **Key Updates**:
  - Added `MOVEHOME` command
  - Added `CONFIG SET homePositionPercent` parameter
  - Enhanced help documentation

### 6. WebInterface (Core 1)
- **Purpose**: Web-based control panel
- **Features**:
  - Responsive HTML5 interface
  - Real-time WebSocket updates
  - Motion control with position feedback
  - Configuration management
  - System diagnostics
  - Stress testing tools
- **Key Updates**:
  - Added "Move to Home" button
  - Fixed stress test to run continuously
  - Enhanced limits tab with validation
  - Improved disabled states and tooltips

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
- **Serial Commands**: 
  - MOVE, MOVEHOME, HOME, STOP, ESTOP
  - CONFIG SET/GET/RESET
  - STATUS, PARAMS, HELP
  - TEST, TEST2 (stress testing)
- **Web Interface**: 
  - Real-time position display
  - Motion control buttons
  - Configuration panels
  - System diagnostics
  - Stress test controls
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

## Latest Implementation (v4.1.0 - February 2025)

### 1. Configurable Home Position
- **Previous**: Fixed position after homing
- **New**: Percentage-based (0-100%) of detected range
- **Benefits**:
  - Adapts to any mechanical installation
  - No code changes needed for different setups
  - User-friendly configuration
  - Scales automatically with detected range

### 2. Enhanced Web Interface
- **Move to Home Button**: One-click return to configured position
- **Improved Limits Tab**: 
  - Shows detected range after homing
  - Validates inputs against hardware limits
  - Clear disabled states when not homed
- **Fixed Stress Test**: Now runs continuously like serial version
- **Better UX**: Tooltips, validation messages, visual feedback

### 3. Serial Interface Updates
- **MOVEHOME Command**: Move to configured home position
- **Configuration**: 
  - `CONFIG SET homePositionPercent <0-100>`
  - `CONFIG RESET homePositionPercent`
- **Help System**: Updated with all new features

### 4. Implementation Details
```cpp
// Calculate home position from percentage
int32_t range = maxPos - minPos;
int32_t homePosition = minPos + (int32_t)((range * config->homePositionPercent) / 100.0f);
```

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
- Available via serial and web interface

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
4. Configure home position percentage if needed
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

## Code Quality & Testing

### Completed Testing
- ✅ Homing sequence with percentage calculation
- ✅ MOVEHOME command functionality
- ✅ Configuration persistence
- ✅ Web interface updates
- ✅ Stress test operation
- ✅ Limit validation
- ✅ Error handling

### Code Standards
- Comprehensive error checking
- Thread-safe operations
- Clear documentation
- Consistent naming conventions
- Modular design

## Future Enhancement Possibilities

While the system is feature-complete, potential future enhancements could include:

### Advanced Motion
- S-curve acceleration profiles
- Multiple saved positions
- Sequence programming
- Synchronized multi-axis control

### Extended Integration
- Encoder feedback for closed-loop control
- MQTT for IoT integration
- REST API for remote control
- Mobile app interface

### Data & Analytics
- Motion history logging
- Predictive maintenance
- Performance analytics
- Cloud backup of configurations

## Conclusion

SkullStepperV4 v4.1.0 represents a complete, production-ready motion control solution. The implementation of percentage-based home positioning was the final feature needed to make the system fully adaptable to any mechanical installation. The modular architecture, comprehensive safety features, and multiple control interfaces make it suitable for professional installations requiring reliable, precise stepper motor control.

The system successfully demonstrates:
- Clean architecture with proper separation of concerns
- Thread-safe multi-core operation
- Robust error handling and recovery
- Professional-grade user interfaces
- Comprehensive configuration management
- Industrial safety standards

This project serves as an excellent reference implementation for ESP32-based motion control systems and can be adapted for various automation applications.
