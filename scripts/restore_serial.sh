#!/bin/bash

# Script to restore SerialInterface.cpp from git
# This will discard any uncommitted changes to this file

echo "============================================"
echo "Restoring SerialInterface.cpp from Git"
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

# Show current status of the file
echo "Current status of SerialInterface.cpp:"
git status SerialInterface.cpp
echo ""

# Show the diff to see what would be lost
echo "Changes that will be lost:"
echo "=========================="
git diff SerialInterface.cpp | head -50
echo ""
echo "... (showing first 50 lines of changes)"
echo ""

# Ask for confirmation
echo "WARNING: This will discard ALL uncommitted changes to SerialInterface.cpp!"
echo "Do you want to restore SerialInterface.cpp from the last commit? (y/n)"
read -r response

if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo ""
    echo "Restoring SerialInterface.cpp..."
    
    # Restore the file from git
    git checkout HEAD -- SerialInterface.cpp
    
    if [ $? -eq 0 ]; then
        echo "✓ SerialInterface.cpp has been restored successfully!"
        echo ""
        echo "File restored from commit:"
        git log -1 --pretty=format:"%h - %s (%cr)" -- SerialInterface.cpp
        echo ""
    else
        echo "✗ Error: Failed to restore SerialInterface.cpp"
        exit 1
    fi
else
    echo "Restoration cancelled."
    exit 0
fi

echo ""
echo "============================================"
echo "Restoration complete!"
echo "============================================"
