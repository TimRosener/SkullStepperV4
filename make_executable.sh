#!/bin/bash

# Make the restore scripts executable
chmod +x /Users/timrosener/Documents/Arduino/SkullStepperV4/restore_serial.sh
chmod +x /Users/timrosener/Documents/Arduino/SkullStepperV4/git_restore_tool.sh

echo "Scripts are now executable!"
echo ""
echo "You can now run:"
echo "  ./restore_serial.sh     - Quick restore from last commit"
echo "  ./git_restore_tool.sh   - Interactive restore with options"
