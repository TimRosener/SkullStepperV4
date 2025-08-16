# Claude.md - AI Assistant Guide for SkullStepperV4

## Project Overview
SkullStepperV4 is a production-ready ESP32-S3 closed-loop stepper control system with professional-grade features. This document provides essential context and guidelines for AI assistants working on this project.

## Critical Development Rules ⚠️

1. **ALWAYS Reference Design Docs First**
   - Read README.md and all design documentation before making ANY changes
   - Understand current phase status and implemented features
   - Verify architecture and modular design principles

2. **NEVER Remove Functionality Without Permission**
   - Ask explicitly before removing ANY features
   - Preserve all existing functionality by default
   - Only enhance existing systems, don't replace them

3. **ALWAYS Update Design Docs When Making Changes**
   - Update README.md with new phase completions
   - Document new features as implemented
   - Keep design docs as the single source of truth

4. **Respect Module Boundaries**
   - Modules NEVER directly call functions from other modules
   - All inter-module communication through queues and protected interfaces
   - Only include GlobalInterface.h in modules

## Project Structure

```
SkullStepperV4/
├── SkullStepperV4.ino          # Main Arduino sketch
├── *.h, *.cpp                  # Source files (flat for Arduino IDE compatibility)
├── docs/                       # Documentation
│   ├── guides/                 # User guides and references
│   │   ├── QUICK_REFERENCE.md
│   │   ├── SerialInterface_Manual.md
│   │   ├── WebInterface_Guide.md
│   │   ├── WebInterface_Design.md
│   │   ├── TESTING_PROTOCOL.md
│   │   └── PUBLISHING.md
│   └── design/                 # Architecture and design docs
│       ├── design_docs.md
│       └── DMX_Implementation_Plan.md
├── scripts/                    # Utility scripts
├── extras/                     # Additional resources
├── README.md                   # Primary documentation (source of truth)
├── CHANGELOG.md                # Version history
├── LICENSE                     # MIT License
└── claude.md                   # This file - AI assistant guide

Key Source Files:
├── GlobalInterface.h           # Shared data structures and interfaces
├── HardwareConfig.h           # Pin definitions and hardware constants
├── GlobalInfrastructure.h/cpp  # Thread-safe infrastructure (mutexes, queues)
├── SystemConfig.h/cpp          # Configuration management (flash storage)
├── SerialInterface.h/cpp       # Serial command processing (Core 1)
├── WebInterface.h/cpp          # Web control interface (Core 1)
├── StepperController.h/cpp     # Motion control (Core 0)
└── DMXReceiver.h/cpp          # DMX512 interface (Core 0)
```

## Key Technical Details

### Hardware Configuration
- **MCU**: ESP32-S3 (dual-core)
- **Stepper Driver**: CL57Y closed-loop driver
- **Control Pins**:
  - STEP: GPIO 7 (Open-drain)
  - DIR: GPIO 15 (Open-drain)
  - ENABLE: GPIO 16 (Open-drain, HIGH = Enabled)
  - ALARM: GPIO 8 (Input, position error from driver)
  - LEFT_LIMIT: GPIO 17 (Active low)
  - RIGHT_LIMIT: GPIO 18 (Active low)
- **DMX**: UART2 on GPIO 4 (RX)

### Core Assignment
- **Core 0**: Real-time operations (StepperController, DMXReceiver, SafetyMonitor)
- **Core 1**: Communication (SerialInterface, WebInterface, SystemConfig)

### Thread Safety
- All shared data protected by FreeRTOS mutexes
- Inter-module communication via queues only
- Use SAFE_READ_STATUS and SAFE_WRITE_STATUS macros
- No direct module-to-module function calls

## Arduino CLI Commands

### Build Commands
```bash
# IMPORTANT: Always use CDCOnBoot=cdc for USB serial communication
# Compile the project with USB CDC enabled
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .

# Upload to board (adjust port as needed) with USB CDC enabled
arduino-cli upload -p /dev/cu.usbmodem11301 --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .

# Monitor serial output
arduino-cli monitor -p /dev/cu.usbmodem11301 -c baudrate=115200

# Combined compile and upload with USB CDC enabled
arduino-cli compile --upload -p /dev/cu.usbmodem11301 --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .
```

### Library Dependencies
```bash
# Install required libraries
arduino-cli lib install "ODStepper"
arduino-cli lib install "FastAccelStepper"
arduino-cli lib install "ESP32 S3 DMX"

# List installed libraries
arduino-cli lib list
```

### Board Configuration
```bash
# Install ESP32 board support
arduino-cli core install esp32:esp32

# Update board index
arduino-cli core update-index

# List connected boards
arduino-cli board list
```

## GitHub Workflow

### Standard Development Flow
```bash
# Check current status
git status

# Create feature branch
git checkout -b feature/description

# Stage changes
git add .

# Commit with descriptive message
git commit -m "Module: Brief description of changes"

# Push to GitHub
git push origin feature/description

# After testing, merge to main
git checkout main
git merge feature/description
git push origin main
```

### Commit Message Format
```
Module: Brief description

- Detailed change 1
- Detailed change 2
- Updated documentation

Fixes #issue_number (if applicable)
```

## Current Development Status

### Completed Modules (Production-Ready)
- ✅ **Phase 1**: Hardware foundation and module framework
- ✅ **Phase 2**: SystemConfig with flash storage
- ✅ **Phase 3**: SerialInterface with human & JSON commands
- ✅ **Phase 4**: StepperController with ODStepper integration
- ✅ **Phase 5**: WebInterface with real-time control

### In Progress
- 🚧 **Phase 6**: DMXReceiver module (Phases 1-5 of 8 complete)
  - Core infrastructure implemented
  - Channel processing implemented
  - Motion integration complete
  - Web interface integration complete (v4.1.11)
  - Remaining: Serial commands, UI enhancements, testing

### Future Phases
- 🔄 **Phase 7**: SafetyMonitor module (optional refactor)
- 🔄 **Phase 8**: Advanced features (logging, OTA updates)

## Common Development Tasks

### Adding a New Command
1. Update SerialInterface.h with command enum
2. Add command parsing in processCommand()
3. Implement command logic
4. Update help text in printHelp()
5. Add to docs/guides/QUICK_REFERENCE.md
6. Update docs/guides/SerialInterface_Manual.md

### Modifying Configuration Parameters
1. Add to SystemConfig struct in GlobalInterface.h
2. Add getter/setter in SystemConfig.cpp
3. Add to loadFromPreferences() and saveToPreferences()
4. Add to JSON export/import functions
5. Add to CONFIG SET command processing
6. Update parameter documentation

### Working with Motion Control
1. Create MotionCommand struct
2. Queue command via g_motionCommandQueue
3. StepperController processes on Core 0
4. Update status via thread-safe macros
5. Never call stepper functions directly from Core 1

### Debugging Tools
- Serial output with verbosity levels (0-3)
- JSON status output for programmatic debugging
- Web interface system info panel
- TEST and TEST2 commands for motion validation
- PARAMS command to verify configuration

## Testing Protocol

### Basic Functionality Test
```
1. Power on system
2. Connect via serial (115200 baud)
3. Run STATUS - verify no faults
4. Run HOME - auto-detect range
5. Run TEST - automated range test
6. Access web interface at 192.168.4.1
7. Verify all controls responsive
```

### Safety Testing
```
1. Trigger limit switches during motion
2. Verify immediate stop
3. Confirm fault latching
4. Test homing clears faults
5. Test emergency stop button
```

## Important Warnings

### Memory Management
- ESP32-S3 has limited RAM
- Avoid dynamic allocation in real-time tasks
- Use static buffers for predictable behavior
- Monitor stack usage in FreeRTOS tasks

### Timing Constraints
- StepperController runs every 2ms on Core 0
- Limit switch polling at 200μs
- Keep ISRs minimal (single instruction)
- Avoid Serial.print in Core 0 tasks

### Flash Wear
- Configuration saves limited by flash endurance
- Batch configuration changes when possible
- Use commitChanges() sparingly
- Monitor flash health in production

## Quick Command Reference

### Essential Commands
- `HOME` - Run auto-range homing (required before motion)
- `MOVE <position>` - Move to absolute position
- `STATUS` - Show system status
- `CONFIG` - Display all configuration
- `HELP` - Show command help

### Configuration
- `CONFIG SET <param> <value>` - Change parameter
- `CONFIG RESET` - Factory reset
- `PARAMS` - List all parameters with ranges

### Testing
- `TEST` - Automated range test
- `TEST2` - Random position test

### Web Interface
- Connect to WiFi: "SkullStepper"
- Browse to: http://192.168.4.1
- Real-time control and monitoring

## Version Information
- **Current Version**: 4.1.14 (2025-02-08)
- **Status**: Production-ready with DMX in development
- **Target Platform**: ESP32-S3
- **Framework**: Arduino with FreeRTOS