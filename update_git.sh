#!/bin/bash

# SkullStepperV4 Git Update Script
# Updates repository with current v4.1.0 production-ready status

echo "============================================"
echo "SkullStepperV4 Git Repository Update"
echo "Version: 4.1.0 - Production Ready"
echo "Date: $(date +"%Y-%m-%d")"
echo "============================================"
echo ""

# Navigate to the project directory
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
echo ""

# Add all changed files
echo "Adding all changed files..."
git add -A

# Create a comprehensive commit message
COMMIT_MESSAGE="feat: Production-ready v4.1.0 with configurable home position

## SkullStepperV4 v4.1.0 - Production Ready ($(date +"%Y-%m-%d"))

### Major Feature: Configurable Home Position

Changed home position from fixed value to percentage-based configuration:
- Home position now set as percentage of detected range (0-100%)
- Automatically scales to any mechanical installation
- Configured via 'CONFIG SET homePositionPercent <value>'
- Default 50% (center of range)
- Persists across power cycles

### New Commands and Features

1. **MOVEHOME Command**
   - Move to the configured home position
   - Requires system to be homed first
   - Shows target position and percentage

2. **Web Interface Updates**
   - Added 'Move to Home' button
   - Fixed stress test to run continuously
   - Enhanced limits tab with validation
   - Improved disabled states and tooltips

3. **Configuration Updates**
   - homePositionPercent parameter (0-100%)
   - CONFIG SET/RESET support
   - Updated help documentation

### Technical Implementation

- GlobalInterface.h: Changed homePosition to homePositionPercent
- SystemConfig: Updated storage/validation for percentage
- StepperController: Calculate position from percentage after homing
- SerialInterface: Added MOVEHOME command and config handlers
- WebInterface: Added button and JavaScript handler

### Benefits

- Adapts to any mechanical setup without code changes
- User-friendly percentage-based configuration
- Consistent behavior across serial and web interfaces
- Safe by design - always within detected limits

### Testing Completed

- Homing sequence with percentage calculation
- Configuration persistence across reboots
- Web interface validation and disabled states
- Serial command compatibility
- Edge cases (0%, 100%, 50%)

This release completes all planned features for SkullStepperV4,
making it fully production-ready for professional installations."

# Commit with the comprehensive message
echo "Committing changes..."
git commit -m "$COMMIT_MESSAGE" || {
    echo "No changes to commit or commit failed"
    exit 1
}

# Show the commit
echo ""
echo "Commit created:"
git log --oneline -1
echo ""

# Show what will be pushed
echo "Changes to be pushed:"
git log origin/main..HEAD --oneline
echo ""

# Ask if user wants to push
echo "Do you want to push to remote? (y/n)"
read -r response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo ""
    echo "Pushing to remote..."
    git push origin main
    if [ $? -eq 0 ]; then
        echo "✓ Push complete!"
    else
        echo "✗ Push failed. Please check your connection and credentials."
        exit 1
    fi
else
    echo "Changes committed locally. Run 'git push' when ready to upload."
fi

echo ""
echo "============================================"
echo "Git update complete!"
echo "Version 4.1.0 is now documented and ready."
echo "============================================"
