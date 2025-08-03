#!/bin/bash

# SkullStepperV4 Git Update Script
# Updates repository with all recent changes

echo "============================================"
echo "SkullStepperV4 Git Repository Update"
echo "============================================"
echo ""

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
COMMIT_MESSAGE="feat: Production-ready v4.1.0 with configurable home position and enhanced web interface

## SkullStepperV4 v4.1.0 - Production Ready (2025-02-02)

This release completes all planned features for the SkullStepperV4 motion control system, making it fully production-ready for professional installations.

## Major Features Completed:

### 1. Configurable Home Position (Percentage-Based)
- **Replaced Fixed Position**: Home position now configured as percentage of detected range (0-100%)
- **Adaptive Scaling**: Automatically adjusts to any mechanical installation
- **Persistent Storage**: Saved to flash and survives power cycles
- **Default 50%**: Centers in range by default, configurable to any percentage
- **Safety Guaranteed**: Always within detected physical limits

### 2. Enhanced Web Interface
- **Move to Home Button**: One-click return to configured home position
- **Improved Limits Tab**: 
  - Shows detected physical range after homing
  - Validates all inputs against hardware limits
  - Disabled until system is homed
  - Clear visual feedback
- **Fixed Stress Test**: TEST button now runs continuously (matches serial behavior)
- **Better UX**: Disabled states, tooltips, and validation messages

### 3. Serial Interface Updates
- **MOVEHOME Command**: Move to configured home position
- **CONFIG SET homePositionPercent**: Set home position (0-100%)
- **Enhanced Help**: Updated documentation for all new features
- **Consistent Behavior**: Serial and web interfaces now functionally identical

### 4. Safety Improvements
- **Percentage Validation**: Home position always within safe bounds
- **Range Enforcement**: Position limits validated against detected range
- **Clear Fault States**: Visual indicators for limit faults
- **Homing Requirements**: Features properly disabled when not homed

## Technical Implementation:

### Code Changes:
- **GlobalInterface.h**: Changed homePosition to homePositionPercent (float)
- **SystemConfig.cpp**: Updated storage/loading for percentage value
- **StepperController.cpp**: Calculate home position from percentage after homing
- **WebInterface.cpp**: 
  - Added Move to Home button with JavaScript handler
  - Enhanced limits configuration with validation
  - Fixed stress test to run continuously
- **SerialInterface.cpp**: Added MOVEHOME command and config handlers

### Architecture:
- Maintains thread-safe operation across dual cores
- Preserves modular design with clean interfaces
- Configuration changes persist through power cycles
- Real-time updates via WebSocket

## Testing Completed:
- ✅ Homing sequence with percentage-based positioning
- ✅ Configuration persistence across reboots
- ✅ Web interface validation and disabled states
- ✅ Stress test continuous operation
- ✅ Move to Home functionality
- ✅ Serial command compatibility

## Documentation Updates:
- Updated README.md with all new features
- Enhanced design_docs.md with current architecture
- Updated project_prompt.md for AI assistant context
- Added examples for new commands

## Production Ready Status:
The system is now feature-complete and ready for deployment:
- All planned features implemented
- Comprehensive testing completed
- Documentation fully updated
- Safety systems verified
- User interfaces polished

## Benefits:
- **Flexible Installation**: Adapts to any mechanical setup
- **User Friendly**: Intuitive web interface with safety guards
- **Professional Grade**: Suitable for commercial installations
- **Maintainable**: Clean architecture and comprehensive docs
- **Reliable**: Extensive error handling and recovery

This release represents the culmination of the SkullStepperV4 development, providing a professional-grade motion control solution ready for real-world deployment."

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
