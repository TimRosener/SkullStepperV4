#!/bin/bash

# Check git status to see what's happening
cd /Users/timrosener/Documents/Arduino/SkullStepperV4

echo "Current git status:"
git status SerialInterface.cpp
echo ""

echo "Checking if file is tracked:"
git ls-files SerialInterface.cpp
echo ""

echo "Last commit that modified SerialInterface.cpp:"
git log -1 --pretty=format:"%h - %s (%cr)" -- SerialInterface.cpp
echo ""

echo "Attempting to force restore from HEAD:"
git checkout HEAD -- SerialInterface.cpp

if [ $? -eq 0 ]; then
    echo "✓ File restored successfully"
    echo ""
    echo "First 10 lines of restored file:"
    head -10 SerialInterface.cpp
else
    echo "✗ Failed to restore file"
    echo ""
    echo "Trying to restore from previous commit:"
    git checkout HEAD~1 -- SerialInterface.cpp
    if [ $? -eq 0 ]; then
        echo "✓ Restored from previous commit"
    else
        echo "✗ Still failing. There may be an issue with the git repository"
    fi
fi
