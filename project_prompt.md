# SkullStepperV4 Project Prompt
## Complete Project Context for AI Assistants
### Version 4.1.1 - Production Ready
### Last Updated: 2025-02-02

## Project Overview

You are working on SkullStepperV4, a production-ready ESP32-S3 based stepper motor control system. This is a complete, modular system with industrial-grade motion control, automatic homing, safety monitoring, and multiple control interfaces (Serial, Web, DMX).

## Current Project Status

**PRODUCTION READY** - All core features implemented and tested:
- ✅ Automatic homing with range detection
- ✅ Position limit enforcement (hardware and software)
- ✅ Configurable motion profiles with jerk limitation
- ✅ Persistent configuration in flash memory
- ✅ Serial command interface (USB and UART)
- ✅ Web interface with real-time updates
- ✅ DMX512 control integration
- ✅ Comprehensive safety monitoring
- ✅ Configurable home position (percentage-based)
- ✅ Stress testing capabilities
- ✅ Emergency stop functionality
- ✅ Home on boot option
- ✅ Home on E-stop option

## Architecture Overview

### Hardware
- **MCU**: ESP32-S3-WROOM-1 (Dual-core, 240MHz)
- **Motion**: TMC2209 stepper driver with UART control
- **Feedback**: Optical limit switches (active LOW)
- **Interface**: USB-C, WiFi AP, DMX512
- **Power**: 24V for motor, 3.3V logic

### Software Architecture
- **Dual-Core Design**:
  - Core 0: Real-time motion control, safety monitoring
  - Core 1: User interfaces, configuration, DMX
- **Thread-Safe**: Mutexes protect shared data
- **Modular**: 7 independent modules with clean interfaces
- **Event-Driven**: Queue-based inter-module communication

## Key Implementation Details

### Motion Control (StepperController)
- Uses FastAccelStepper library for smooth motion
- Automatic homing sequence:
  1. Find left limit (set as 0)
  2. Find right limit
  3. Calculate range with safety margins
  4. Move to configured home position (percentage)
- Position limits enforced in software
- Motion profiles with speed/acceleration/jerk control

### Safety System
- Hardware limit switches monitored continuously
- Software position limits within hardware bounds
- Limit fault detection requires homing to clear
- Emergency stop with high deceleration
- Stepper driver alarm monitoring

### Configuration System
- ESP32 Preferences (flash) for persistent storage
- All parameters survive power cycles
- Runtime configuration without restart
- JSON import/export capability
- Home position as percentage of range (0-100%)
- Home on boot option for automatic initialization
- Home on E-stop option for automatic recovery

### User Interfaces

#### Serial Interface
- Human-readable commands (MOVE, HOME, STOP, etc.)
- JSON API for automation
- Real-time status updates
- Comprehensive help system
- New MOVEHOME command

#### Web Interface
- Responsive HTML5 design
- WebSocket for real-time updates
- Motion control panel
- Configuration management
- System diagnostics
- "Move to Home" button
- Stress testing tools

#### DMX Interface
- 512 channel support
- Position control via DMX value
- Configurable scaling and offset
- Signal timeout detection

## Recent Changes (2025-02-02)

1. **Fixed Web Stress Test**: Now runs continuously like serial version
2. **Enhanced Limits Tab**: Shows detected range, validates inputs
3. **Configurable Home Position**: Percentage-based (0-100%) instead of fixed
4. **Move to Home Button**: One-click return to configured position
5. **Improved UI/UX**: Better feedback and disabled states
6. **Home on Boot**: Checkbox option to automatically home the system on startup
7. **Home on E-Stop**: Checkbox option to automatically home after emergency stop
8. **Fixed Auto-Home on E-Stop**: Corrected implementation - now properly triggers from COMPLETE state and clears limit fault

## Code Organization

```
SkullStepperV4/
├── SkullStepperV4.ino      # Main entry point
├── GlobalInterface.h        # Shared data structures
├── GlobalInfrastructure.cpp # Thread-safe infrastructure
├── HardwareConfig.h         # Pin definitions
├── StepperController.cpp/h  # Motion control (Core 0)
├── SafetyMonitor.cpp/h      # Limit/fault monitoring (Core 0)
├── SystemConfig.cpp/h       # Configuration management (Core 1)
├── SerialInterface.cpp/h    # Serial commands (Core 1)
├── WebInterface.cpp/h       # Web server (Core 1)
├── DMXReceiver.cpp/h        # DMX control (Core 1)
└── README.md               # Comprehensive documentation
```

## Development Guidelines

### Adding Features
1. Identify which module owns the functionality
2. Maintain thread safety (use provided macros)
3. Update both serial and web interfaces
4. Add configuration parameters if needed
5. Document changes in README.md

### Thread Safety Rules
- Use SAFE_READ_STATUS/SAFE_WRITE_STATUS macros
- Never access g_systemStatus directly
- Queue commands between cores
- Keep critical sections short

### Testing Requirements
- Test normal operation
- Test error conditions
- Verify limit switch behavior
- Check configuration persistence
- Validate web interface updates

## Common Tasks

### Adding a New Command
1. Add to SerialInterface::processCommand()
2. Create handler function
3. Update help text
4. Add to web interface if applicable
5. Document in README.md

### Adding Configuration Parameter
1. Add to SystemConfig structure
2. Update load/save functions
3. Add validation
4. Update web interface
5. Add serial CONFIG SET/RESET handlers

### Debugging
- Serial output at 115200 baud
- Web interface shows real-time status
- CONFIG SHOW displays all parameters
- STATUS shows current state
- Test commands help isolate issues

## Current Focus Areas

The system is production-ready. Recent bug fixes:
- Fixed auto-home on E-stop to properly initiate homing sequence
- Auto-home now correctly uses processMotionCommand() instead of direct function calls
- Auto-home triggers correctly from both IDLE and COMPLETE homing states
- Limit fault is automatically cleared to allow auto-home to proceed

Future enhancements could include:
- Multiple motion profiles
- Position presets
- Sequence programming
- Data logging
- Encoder feedback

## Important Notes

1. **Homing Required**: Many features require homing first
2. **Percentage Home**: Home position is now percentage-based
3. **Limit Faults**: Require homing to clear
4. **Configuration**: Saved to flash automatically
5. **Web Interface**: Available at 192.168.4.1 in AP mode
6. **Auto-Homing**: Can be configured for boot and E-stop recovery

## Getting Help

- Check README.md for comprehensive documentation
- Use HELP command for serial interface guide
- Web interface has built-in parameter descriptions
- Code comments explain implementation details
- Design docs provide architecture overview

This system is ready for production deployment in professional installations requiring reliable, safe stepper motor control with multiple interface options.
