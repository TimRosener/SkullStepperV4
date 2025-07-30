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
COMMIT_MESSAGE="Major system updates: Enhanced noise filtering, new commands, and bug fixes

## Hardware Updates:
- Increased LIMIT_SWITCH_DEBOUNCE from 50ms to 100ms for better noise immunity
- Added hardware filtering recommendations (RC filters: 1kΩ + 0.1µF)
- Updated pin documentation with confirmed assignments (LEFT: GPIO 17, RIGHT: GPIO 18, ALARM: GPIO 8)

## StepperController Enhancements:
- Fixed critical limit switch bug: Now properly stops motion when limits are active during normal operation
- Added multi-sample validation requiring 3 consecutive stable readings for noise filtering
- Separated limit state detection from emergency stop logic for continuous monitoring
- Improved homing state machine with better error handling
- Extended homing timeout from 30s to 90s for longer travel distances
- Motor now remains enabled by default for position holding (setAutoEnable(false))

## SerialInterface New Features:
- Added PARAMS command to list all configurable parameters with ranges and descriptions
- Added TEST command for automated range testing (moves between 10% and 90% of homed range)
- TEST command includes safety check requiring successful homing before running
- Enhanced help system with complete command documentation
- Fixed command processing to handle test interruption properly

## Global Interface Updates:
- Added missing StepperController function declarations (isHomed, getPositionLimits, etc.)
- Synchronized function declarations between GlobalInterface.h and StepperController.h
- Added homing and advanced motion function declarations

## Documentation:
- Created comprehensive limit switch wiring guide
- Added EMI/noise mitigation strategies
- Updated README with current development status
- Added parameter range documentation
- Updated README with Phase 4 major progress status
- Added complete command reference section
- Added next steps for project continuation
- Documented all recent fixes and known issues

## Bug Fixes:
- Fixed limit switch emergency stop not triggering during continuous activation
- Fixed compilation errors for missing function declarations
- Corrected noise filtering logic in handleLimitFlags()
- Fixed test command to properly check for key press interruption

## Safety Improvements:
- Limit switches now continuously monitored every 1ms
- Emergency stop triggers immediately on limit activation outside of homing
- Added clear debug messages for emergency stop events
- Test command validates system is homed before allowing operation

## Project Status:
- Phase 4 (StepperController with ODStepper) now has MAJOR PROGRESS
- System ready for production testing with complete motion control
- Auto-range homing fully functional
- Comprehensive command interface operational

This commit represents Phase 4 near completion with enhanced safety, usability features, and updated documentation."

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
