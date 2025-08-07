# SkullStepperV4 Project Prompt
## Complete Project Context for AI Assistants
### Version 4.1.6 - Production Ready with DMX Development
### Last Updated: 2025-02-02

## Project Overview

You are working on SkullStepperV4, a production-ready ESP32-S3 based stepper motor control system. This is a complete, modular system with industrial-grade motion control, automatic homing, safety monitoring, and multiple control interfaces (Serial, Web, DMX).

## Current Project Status

**PRODUCTION READY** with DMX development in progress:
- ✅ Automatic homing with range detection
- ✅ Position limit enforcement (hardware and software)
- ✅ Configurable motion profiles with jerk limitation
- ✅ Persistent configuration in flash memory
- ✅ Serial command interface (USB and UART)
- ✅ Web interface with real-time updates
- 🚧 DMX512 control integration (Phase 1 of 8 complete)
- ✅ Comprehensive safety monitoring
- ✅ Configurable home position (percentage-based)
- ✅ Stress testing capabilities
- ✅ Emergency stop functionality
- ✅ Home on boot option
- ✅ Home on E-stop option

## DMX Implementation Status (Phase 6)

### Completed (Phases 1-4 COMPLETE)
- ✅ **Phase 1**: Core DMX Infrastructure
  - ESP32S3DMX library integration on UART2/GPIO4
  - Core 0 real-time task for DMX processing (10ms cycle)
  - 5-channel cache system with improved layout
  - Signal loss detection with configurable timeout
  - Base channel configuration (1-508)
- ✅ **Phase 2**: Channel Processing
  - Position mapping (8-bit and 16-bit modes)
  - Speed/acceleration scaling (0-100% of max)
  - Mode detection with hysteresis (Stop/Control/Home)
- ✅ **Phase 3**: System Integration
  - Full StepperController integration
  - Dynamic parameter updates
  - Thread-safe command queuing
- ✅ **Phase 4**: Motion Integration  
  - DMX controls position, speed, and acceleration
  - Smooth mode transitions
  - Signal loss position hold

### Remaining DMX Phases
- Phase 5: Web Interface Updates (DMX display/config)
- Phase 6: Serial Interface Updates (DMX commands)
- Phase 7: 16-bit Position Implementation UI
- Phase 8: Testing & Validation

## Architecture Overview

### Hardware
- **MCU**: ESP32-S3-WROOM-1 (Dual-core, 240MHz)
- **Motion**: CL57Y closed-loop stepper driver
- **DMX**: ESP32S3DMX library on UART2 (GPIO 4)
- **Feedback**: Optical limit switches (active LOW)
- **Interface**: USB-C, WiFi AP, DMX512
- **Power**: 24V for motor, 3.3V logic

### Software Architecture
- **Dual-Core Design**:
  - Core 0: Real-time motion control, safety monitoring, DMX reception
  - Core 1: User interfaces, configuration
- **Thread-Safe**: Mutexes protect shared data
- **Modular**: 7 independent modules with clean interfaces
- **Event-Driven**: Queue-based inter-module communication

## Key Implementation Details

### Motion Control (StepperController)
- Uses ODStepper/FastAccelStepper library for smooth motion
- Automatic homing sequence:
  1. Find left limit (set as 0)
  2. Find right limit
  3. Calculate range with safety margins
  4. Move to configured home position (percentage)
- Position limits enforced in software
- Motion profiles with speed/acceleration/jerk control

### DMX Control (DMXReceiver) - IN DEVELOPMENT
- ESP32S3DMX library for reliable DMX512 reception
- 5-channel operation with base offset:
  - Position control (8-bit or 16-bit precision)
  - Dynamic speed/acceleration limiting
  - Mode control (Stop/DMX/Home)
- Real-time processing on Core 0
- Signal timeout with position hold
- Configurable base channel (1-508)

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

#### DMX Interface (In Development)
- 5-channel control scheme
- Position control with 8/16-bit modes
- Dynamic parameter adjustment
- Mode switching capability
- Signal monitoring

## Recent Changes (2025-02-02)

1. **DMX Phase 1 Complete**: Core infrastructure with ESP32S3DMX
2. **Improved DMX Channel Layout**: Position LSB moved to channel 1
3. **Fixed Web Stress Test**: Now runs continuously like serial version
4. **Enhanced Limits Tab**: Shows detected range, validates inputs
5. **Configurable Home Position**: Percentage-based (0-100%) instead of fixed
6. **Move to Home Button**: One-click return to configured position
7. **Home on Boot/E-Stop**: Automatic homing options
8. **Fixed Auto-Home on E-Stop**: Corrected implementation
9. **Web Interface Improvements (v4.1.4)**:
   - Consolidated homing warnings into single message
   - Configuration can be changed without homing (only movement restricted)
   - Reorganized tabs: removed Advanced, redistributed settings logically
   - Motion settings (including jerk, emergency decel) in Motion & Limits tab
   - All DMX settings (including timeout) in DMX tab
10. **Fixed Checkbox Visibility (v4.1.5)**:
   - Auto-Home on Boot and Auto-Home on E-Stop now always visible
   - Created dedicated "Homing Options" section in Motion & Limits tab
   - Checkboxes no longer hidden when system is not homed
11. **DMX Safety Enhancement (v4.1.6)**:
   - DMX position control now blocked when homing is required
   - DMX HOME command (channel 5 = 171-255) still works when homing needed
   - **DMX input completely ignored during homing sequence**
   - **Homing only needs momentary DMX trigger (prevents loops)**
   - Allows DMX to initiate homing even after E-STOP or on boot
   - Clear console messages indicate when DMX control is blocked

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
├── DMXReceiver.cpp/h        # DMX control (Core 0) - ACTIVE DEVELOPMENT
├── DMX_Implementation_Plan.md # DMX development roadmap
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

### DMX Development Guidelines
- Follow the DMX_Implementation_Plan.md phases
- Maintain Core 0 real-time constraints
- Use existing thread-safe infrastructure
- Test with various DMX sources
- Document channel assignments clearly

### Testing Requirements
- Test normal operation
- Test error conditions
- Verify limit switch behavior
- Check configuration persistence
- Validate web interface updates
- Test DMX signal reception (when complete)

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

### Working with DMX
1. Check DMX_Implementation_Plan.md for design
2. Use DMXReceiver namespace functions
3. Access channel cache for processed values
4. Monitor signal state before using data
5. Test with DMX console

### Debugging
- Serial output at 115200 baud
- Web interface shows real-time status
- CONFIG SHOW displays all parameters
- STATUS shows current state
- Test commands help isolate issues
- DMX monitoring (coming in Phase 6)

## Current Focus Areas

The system is production-ready with DMX development in progress:
- DMX Phase 1 (Core Infrastructure) ✅ COMPLETE
- DMX Phase 2 (Channel Processing) - Next priority
- Integration with existing motion control
- Web interface DMX status display
- Serial commands for DMX configuration

Future enhancements:
- Complete DMX implementation (Phases 2-8)
- Multiple motion profiles
- Position presets
- Sequence programming
- Data logging

## Important Notes

1. **Homing Required**: Many features require homing first
2. **Percentage Home**: Home position is now percentage-based
3. **Limit Faults**: Require homing to clear
4. **Configuration**: Saved to flash automatically
5. **Web Interface**: Available at 192.168.4.1 in AP mode
6. **Auto-Homing**: Can be configured for boot and E-stop recovery
7. **DMX Development**: Following structured 8-phase plan

## Getting Help

- Check README.md for comprehensive documentation
- Use HELP command for serial interface guide
- Web interface has built-in parameter descriptions
- Code comments explain implementation details
- Design docs provide architecture overview
- DMX_Implementation_Plan.md for DMX development

This system is ready for production deployment in professional installations requiring reliable, safe stepper motor control with multiple interface options. DMX control is actively being developed to add professional lighting console integration.
