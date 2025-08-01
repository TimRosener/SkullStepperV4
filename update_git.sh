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
COMMIT_MESSAGE="Configuration system enhancements: Persistence fixes and user-configurable homing speed

## Configuration Persistence Fix (2025-01-31):

### Fixed Configuration Loading on Boot:
- StepperController now properly loads saved configuration from flash during initialization
- Added SystemConfig include and getConfig() call in StepperController::initialize()
- Motion parameters (maxSpeed, acceleration, jerk, enableLimits) now persist across reboots
- System uses saved values instead of hardcoded defaults from HardwareConfig.h
- Added diagnostic output confirming loaded values on startup
- Fallback to defaults if configuration load fails

### Benefits:
- User settings are preserved through power cycles
- No need to reconfigure after each boot
- Consistent behavior with saved preferences
- Clear feedback about loaded configuration

## User-Configurable Homing Speed (2025-01-31):

### Added Homing Speed Parameter:
- Homing speed is now a user-configurable parameter (was hardcoded at 940 steps/sec)
- Added 'homingSpeed' to SystemConfig structure in GlobalInterface.h
- Integrated with flash storage for persistence
- Range: 0-10000 steps/sec, Default: 940 steps/sec
- Accessible via 'CONFIG SET homingSpeed <value>' command

### Implementation Details:
- SystemConfig.cpp: Added to default config, load/save functions, and JSON export/import
- StepperController.cpp: Changed from const HOMING_SPEED to g_homingSpeed variable
- StepperController loads homing speed from config on initialization
- SerialInterface.cpp: Added CONFIG SET/RESET commands for homingSpeed
- Added to PARAMS command output with usage examples
- Included in CONFIG JSON output with metadata (range, units, description)
- Added to 'motion' parameter group for bulk reset

### User Interface:
- CONFIG SET homingSpeed 1500    # Set homing speed to 1500 steps/sec
- CONFIG RESET homingSpeed       # Reset to default (940 steps/sec)
- CONFIG RESET motion            # Reset all motion params including homingSpeed
- PARAMS                         # Shows homingSpeed in parameter list
- CONFIG                         # Displays current homing speed value

## Documentation Updates:
- Updated README.md with configuration persistence information
- Added homingSpeed to configuration parameter table
- Updated Phase 4 accomplishments with new features
- Added recent development notes for both enhancements

## Testing Recommendations:
1. Set custom configuration values (maxSpeed, acceleration, homingSpeed)
2. Reboot system and verify values persist
3. Run HOME command with different homingSpeed settings
4. Verify smooth operation at various homing speeds
5. Test CONFIG RESET functionality for individual and group resets

This update completes the configuration system with full persistence and adds flexibility to the homing sequence speed control."

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
