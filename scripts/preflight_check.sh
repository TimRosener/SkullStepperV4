#!/bin/bash

# ============================================================================
# File: preflight_check.sh
# Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
# Description: Pre-flight checklist for validating system before commits
# Author: Tim Rosener
# ============================================================================

echo "============================================================================"
echo "SkullStepperV4 Pre-flight Checklist"
echo "============================================================================"
echo ""

# Color codes for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Track overall status
ALL_GOOD=true

# Function to check file exists
check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓${NC} $2"
        return 0
    else
        echo -e "${RED}✗${NC} $2 - File missing: $1"
        ALL_GOOD=false
        return 1
    fi
}

# Function to check version consistency
check_version() {
    VERSION="4.1.0"
    if grep -q "$VERSION" "$1" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Version $VERSION found in $2"
        return 0
    else
        echo -e "${YELLOW}⚠${NC} Version $VERSION not found in $2"
        ALL_GOOD=false
        return 1
    fi
}

echo "Checking required files..."
echo "--------------------------"
check_file "SkullStepperV4.ino" "Main sketch file"
check_file "README.md" "Project README"
check_file "design_docs.md" "Design documentation"
check_file "PUBLISHING.md" "Publishing guide"
check_file "CHANGELOG.md" "Change log"
check_file "GlobalInterface.h" "Global interface header"
check_file "StepperController.cpp" "Stepper controller implementation"
check_file "SerialInterface.cpp" "Serial interface implementation"
check_file "SystemConfig.cpp" "System configuration implementation"
check_file "WebInterface.cpp" "Web interface implementation"

echo ""
echo "Checking version consistency..."
echo "-------------------------------"
check_version "SkullStepperV4.ino" "Main sketch"
check_version "README.md" "README"
check_version "design_docs.md" "Design docs"

echo ""
echo "Checking documentation..."
echo "-------------------------"
# Check if README has current status
if grep -q "Current Status Summary" README.md && grep -q "Production-ready" README.md; then
    echo -e "${GREEN}✓${NC} README appears up to date"
else
    echo -e "${YELLOW}⚠${NC} README may need updating"
fi

# Check if design docs are current
if grep -q "Production Ready v4.1.0" design_docs.md; then
    echo -e "${GREEN}✓${NC} Design docs appear current"
else
    echo -e "${YELLOW}⚠${NC} Design docs may need updating"
fi

echo ""
echo "Checking executable permissions..."
echo "---------------------------------"
for script in *.sh; do
    if [ -x "$script" ]; then
        echo -e "${GREEN}✓${NC} $script is executable"
    else
        echo -e "${YELLOW}⚠${NC} $script is not executable (run ./make_executable.sh)"
    fi
done

echo ""
echo "Reminders before committing:"
echo "---------------------------"
echo "[ ] Have you tested the homing sequence?"
echo "[ ] Have you verified serial commands work?"
echo "[ ] Have you checked the web interface?"
echo "[ ] Have you tested limit switches?"
echo "[ ] Have you verified configuration persistence?"
echo "[ ] Have you run TEST and TEST2 commands?"
echo "[ ] Have you updated documentation if needed?"
echo "[ ] Have you removed any sensitive information?"

echo ""
if [ "$ALL_GOOD" = true ]; then
    echo -e "${GREEN}Pre-flight check passed!${NC}"
    echo "Remember to test hardware functionality before committing."
else
    echo -e "${RED}Pre-flight check found issues.${NC}"
    echo "Please address the issues above before committing."
fi

echo ""
echo "To commit your changes:"
echo "  ./update_git.sh \"Your descriptive commit message\""
echo ""
echo "============================================================================"
