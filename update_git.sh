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
COMMIT_MESSAGE="feat: Add advanced configuration parameters to WebInterface

## WebInterface Enhancement (2025-02-02):

Completed Section 3 (Advanced Configuration) by adding all missing advanced parameters to the web interface.

## Implementation Details:

### New Advanced Configuration Tab:
- **Jerk Control**: Range slider (0-50000 steps/sec³) with live value display
- **Emergency Deceleration**: Range slider (100-50000 steps/sec²) for emergency stop rate
- **DMX Timeout**: Number input (100-60000 ms) for signal loss detection
- **Parameter Help Text**: Added descriptive info for each advanced parameter

### Technical Implementation:
- Added new "Advanced" tab to configuration section
- Extended JavaScript event handlers for new sliders with touch support
- Updated applyConfig() to include jerk, emergencyDeceleration, and dmxTimeout
- Enhanced getSystemStatus() to send all parameters via WebSocket
- Modified updateConfiguration() to handle new parameter updates
- Added .param-info CSS class for parameter descriptions

### Backend Integration:
- All parameters properly saved to flash via SystemConfigMgr
- Thread-safe configuration updates using existing patterns
- Full integration with motion control system
- Maintains compatibility with serial interface

### Code Changes:
- **WebInterface.cpp**:
  - Added HTML for Advanced tab with three new controls
  - Added CSS styling for parameter info text
  - Extended JavaScript for slider interactions
  - Updated C++ handlers for new parameters
  - Enhanced configuration structures
- **webfuncstat.md**:
  - Marked Section 3 (Advanced Configuration) as complete ✅
  - Updated feature comparison documentation

## Architecture Compliance:

- ✅ No direct module calls - uses SystemConfigMgr interface
- ✅ Thread-safe access via existing configuration system
- ✅ No functionality removed - only enhancements
- ✅ Documentation fully updated
- ✅ Consistent UI/UX with existing design
- ✅ Mobile-responsive implementation

## User Benefits:

- Complete control over motion smoothness via jerk limitation
- Configurable emergency stop behavior for safety
- DMX timeout adjustment for different DMX sources
- Parameter validation and range enforcement
- Persistent storage across power cycles

The WebInterface now provides access to ALL system parameters, matching the capabilities of the SerialInterface for advanced users."

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
