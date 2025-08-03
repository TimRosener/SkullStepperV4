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
COMMIT_MESSAGE="fix: WebInterface stress test now runs continuously + UI improvements

## WebInterface Fixes and Enhancements (2025-02-02):

Fixed the TEST function in WebInterface to properly implement a continuous stress test, matching the behavior of the SerialInterface.

## Key Changes:

### Stress Test Implementation:
- **Added Continuous Testing**: TEST button now runs continuously between 10% and 90% of range
- **State Tracking**: Added test state variables to track active tests and movement progress
- **Auto-Movement**: System automatically sends next move command when current move completes
- **Progress Updates**: Shows test cycle count every 10 movements via WebSocket
- **Proper Stop Handling**: STOP and E-STOP buttons properly terminate active tests

### Random Test Enhancement:
- **Renamed TEST2 to RANDOM MOVES**: More descriptive name for better user clarity
- **10 Position Sequence**: Generates and visits 10 random positions within safe range
- **Progress Tracking**: Reports current position (e.g., \"Moving to position 3 of 10\")
- **Completion Notification**: Announces when all positions have been visited

### Technical Implementation:
- Added updateStressTest() and updateRandomTest() methods called from main update loop
- Test state checks every 100ms for responsive behavior
- Integrated with existing motion control queue system
- Thread-safe implementation using existing patterns
- WebSocket broadcasts for real-time test status updates

### Safety Features:
- Tests automatically stop on limit fault detection
- Clear error messages sent to all connected clients
- Requires homing before test execution
- Uses 10%-90% of detected range for safety margins

### UI Improvements:
- Changed TEST button label to \"STRESS TEST\" for clarity
- Updated tooltips with accurate descriptions
- Modified help text: \"Use STOP or E-STOP buttons to stop tests\"
- Button styling and layout remains consistent

## Code Changes:
- **WebInterface.cpp**: 
  - Added test state variables and update functions
  - Modified command handlers for continuous operation
  - Enhanced WebSocket message handling
  - Updated UI labels and help text
- **WebInterface.h**: Added updateStressTest() and updateRandomTest() declarations
- **README.md**: Documented the fixes and improvements

## Benefits:
- Web interface test functions now match serial interface behavior
- True stress testing capability for mechanical validation
- Better user understanding through clearer button labels
- Consistent test behavior across all interfaces

The stress test will now run indefinitely until stopped, making it useful for burn-in testing, vibration analysis, and mechanical validation."

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
