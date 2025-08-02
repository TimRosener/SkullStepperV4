#!/bin/bash

# SkullStepperV4 Git Update Script
# Updates repository with all recent changes

echo "============================================"
echo "SkullStepperV4 Git Repository Update"
echo "============================================"

# Navigate to the project directory (adjust path as needed)
cd /Users/timrosener/Documents/Arduino/SkullStepperV4

# Check if we're in a git repository
if [ ! -d .git ]; then
    echo "Error: Not in a git repository!"
    echo "Please run this script from the SkullStepperV4 directory"
    exit 1
fi

# Show current status
echo "Current git status:"
git status --short

# Add all changed files
echo -e "\nAdding all changed files..."
git add .

# Create a comprehensive commit message
COMMIT_MESSAGE="v4.1.0: Production-Ready System with Complete Web Interface Integration

## Major Milestone Achieved (2025-02-02):

The SkullStepperV4 system is now **production-ready** with a comprehensive web-based control interface alongside the existing serial command system. All core functionality is implemented, tested, and documented.

## Phase 5: WebInterface Module (COMPLETE):

### Web Server Implementation:
- **WiFi Access Point**: Standalone network 'SkullStepper' (no router required)
- **ESP32 WebServer**: Built-in web server on port 80
- **WebSocket Server**: Real-time bidirectional communication on port 81
- **Captive Portal**: Automatic redirect when connecting to WiFi
- **Thread-Safe Design**: Proper integration with existing dual-core architecture

### User Interface Features:
- **Responsive Design**: Mobile-friendly interface with touch controls
- **Real-Time Updates**: 10Hz WebSocket updates for smooth UI response
- **Motion Control**: Move, jog, home, stop, and emergency stop buttons
- **Live Position Display**: Current position, speed, and status indicators
- **Limit Switch Status**: Visual indicators for hardware limits
- **Configuration Control**: Real-time speed and acceleration adjustment
- **Test Functions**: TEST and TEST2 buttons for system validation
- **Connection Management**: Supports up to 2 simultaneous WebSocket clients

### Technical Implementation:
- **Zero External Dependencies**: All HTML/CSS/JS embedded in firmware
- **PROGMEM Storage**: Efficient memory usage for web assets
- **JSON Communication**: WebSocket messages use JSON format
- **Queue Integration**: Uses existing motion command queue
- **Status Broadcasting**: Automatic updates to all connected clients
- **REST API**: HTTP endpoints for automation integration

### Safety Enhancements:
- **EMERGENCY_STOP State**: Limit faults now trigger proper emergency stop
- **Fault Display**: Clear indication of limit fault conditions
- **Homing Requirement**: Web UI enforces homing before movement
- **Command Validation**: All commands validated before execution

## System Status Update:

### Completed Phases:
✅ Phase 1: Hardware foundation and module framework
✅ Phase 2: Configuration management with flash storage
✅ Phase 3: Interactive command interface (human & JSON)
✅ Phase 4: Motion control with ODStepper integration
✅ Phase 5: WebInterface module (now core system component)

### Production Features:
- **Dual Control Interfaces**: Serial commands and web control
- **Industrial Motion Control**: Hardware timer-based step generation
- **Auto-Range Homing**: Adapts to any mechanical installation
- **Configuration Persistence**: All settings saved to flash
- **Thread-Safe Operation**: Dual-core FreeRTOS architecture
- **Comprehensive Safety**: Hardware limits with fault latching
- **Built-in Diagnostics**: TEST and TEST2 validation routines

## Documentation Updates:
- Updated README.md to v4.1.0 with production-ready status
- Moved WebInterface from Phase 7 to Phase 5 (implemented)
- Added complete WebInterface feature documentation
- Updated system status to reflect all completed functionality
- Enhanced safety documentation with EMERGENCY_STOP behavior
- Added web interface access instructions (192.168.4.1)

## Testing Performed:
1. Web interface tested on multiple devices (desktop, mobile)
2. WebSocket real-time updates verified at 10Hz
3. All motion commands tested via web interface
4. Configuration changes persist across reboots
5. Limit fault handling verified with proper state transitions
6. Simultaneous serial and web control verified
7. TEST and TEST2 functions work from both interfaces

## Usage Instructions:
1. Power on the SkullStepper system
2. Connect to WiFi network 'SkullStepper'
3. Browse to http://192.168.4.1
4. Run HOME command to initialize
5. Use web interface for motion control and configuration

## Next Development Phase:
Phase 6: DMXReceiver module for DMX512 integration

This release marks the completion of all core functionality, making SkullStepperV4 a professional-grade stepper control system suitable for production use."

# Commit with the comprehensive message
echo -e "\nCommitting changes..."
git commit -m "$COMMIT_MESSAGE"

# Show the commit
echo -e "\nCommit created:"
git log --oneline -1

# Ask if user wants to push
echo -e "\nDo you want to push to remote? (y/n)"
read -r response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo "Pushing to remote..."
    git push
    echo "Push complete!"
else
    echo "Changes committed locally. Run 'git push' when ready to upload."
fi

echo -e "\n============================================"
echo "Git update complete!"
echo "============================================"
