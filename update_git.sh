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
COMMIT_MESSAGE="feat: Add system information display to WebInterface

## WebInterface Enhancement (2025-02-02):

Added comprehensive system information display panel to the web interface, completing Section 2 of the missing features list.

## Implementation Details:

### New System Information Panel:
- **Version Display**: Shows firmware version (4.1.0)
- **Hardware Info**: Displays ESP32-S3-WROOM-1 model
- **Uptime Monitor**: Real-time uptime with human-readable formatting (days/hours/minutes/seconds)
- **Memory Statistics**: Free heap memory display with KB/MB formatting
- **Task Monitoring**: FreeRTOS task stack high water mark for broadcast task
- **Connection Status**: Active WebSocket clients count with max limit display

### Technical Implementation:
- Extended getSystemStatus() to include systemInfo object in WebSocket updates
- Added formatUptime() helper function for time formatting
- Added formatMemory() helper function for memory size formatting
- Created new CSS classes: .info-grid and .info-item matching existing design
- Updates delivered via existing 10Hz WebSocket infrastructure
- Thread-safe data access using established patterns

### Code Changes:
- **WebInterface.cpp**: 
  - Added HTML structure for system info panel
  - Added CSS styling for info grid layout
  - Added JavaScript helper functions and UI update logic
  - Extended getSystemStatus() with system information
  - Updated getSystemInfo() with additional details
- **README.md**: 
  - Updated WebInterface features list
  - Corrected 'Async Web Server' to 'Built-in Web Server'
  - Added WebSocket port 81 specification
  - Added system info display to latest enhancements
- **webfuncstat.md**: 
  - Marked Section 2 (Information Display) as complete ✅
  - Listed all implemented info display features

## Architecture Compliance:

- ✅ Follows modular architecture - no direct module calls
- ✅ Uses thread-safe data access patterns
- ✅ Maintains dual-core architecture principles
- ✅ No functionality removed - only enhancements added
- ✅ Documentation updated to reflect changes
- ✅ Consistent with existing UI/UX design

## Testing Notes:

System information panel tested and working:
- Uptime increments correctly in real-time
- Memory usage reflects actual ESP32 heap status
- Task stack monitoring provides useful debugging info
- All formatting functions handle edge cases properly

This enhancement improves system monitoring capabilities while maintaining the clean, professional interface design."

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
