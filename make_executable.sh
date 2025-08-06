#!/bin/bash

# ============================================================================
# File: make_executable.sh
# Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
# Description: Makes all shell scripts in the project executable
# ============================================================================

echo "Making all shell scripts executable..."
echo ""

# Make all .sh files executable
for script in *.sh; do
    if [ -f "$script" ]; then
        chmod +x "$script"
        echo "✓ Made $script executable"
    fi
done

echo ""
echo "All scripts are now executable!"
echo ""
echo "Available scripts:"
echo "  ./update_git.sh         - Commit and push changes"
echo "  ./git_restore_tool.sh   - Interactive git management"
echo "  ./fix_serial.sh         - Disable verbose serial output"
echo "  ./restore_serial.sh     - Restore serial output"
echo "  ./preflight_check.sh    - Pre-commit validation"
echo "  ./make_executable.sh    - This script"
echo ""
echo "Run ./preflight_check.sh before committing changes!"
