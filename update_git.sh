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
COMMIT_MESSAGE="Documentation Update: WebInterface Module Now Production-Ready (Phase 5 Complete)

## Documentation Updates (2025-02-02):

Updated all project documentation to reflect that the WebInterface module is now fully implemented and integrated as a core system component.

## Major Documentation Changes:

### README.md Updates:
- Changed StepperController status from 'IN PROGRESS' to 'COMPLETE'
- Moved WebInterface from Phase 7 to Phase 5 (correctly ordered)
- Updated phase summary: Phases 1-5 now all COMPLETE
- Updated project status to v4.1.0 Production-Ready
- Clarified DMXReceiver as next priority (Phase 6)
- Marked SafetyMonitor as optional (Phase 7) since safety is already integrated

### WebInterface_Design.md → WebInterface Documentation:
- Renamed from 'Design Document' to 'Documentation' 
- Changed from 'Phase 7' to 'Phase 5 (IMPLEMENTED)'
- Added status note: 'FULLY IMPLEMENTED and integrated into the production system'
- Updated all implementation phases to show completed checkmarks
- Changed 'Required Libraries' to 'Implemented Libraries'
- Updated library section to reflect actual implementation:
  - ESP32 WebServer (built-in)
  - WebSocketsServer by Markus Sattler
  - ArduinoJson
  - DNSServer (built-in)
- Updated conclusion to reflect completed implementation
- Incremented document version to v2.0.0

### ChatPrompt.md Updates:
- Clarified system has 'dual interfaces (Serial & Web)'
- Confirmed production-ready status with all features complete

## Current System Status:

### Completed Modules:
✅ Phase 1: Hardware foundation and module framework
✅ Phase 2: Configuration management with flash storage  
✅ Phase 3: Interactive command interface (human & JSON)
✅ Phase 4: Motion control with ODStepper integration
✅ Phase 5: WebInterface module (WiFi control interface)

### Future Development:
📝 Phase 6: DMXReceiver module - NEXT PRIORITY
🔄 Phase 7: SafetyMonitor module - OPTIONAL (safety already integrated)
🔄 Phase 8: Advanced features - FUTURE

## Key Clarifications:

1. **WebInterface is NOT an alternative** - it's a fully integrated core component
2. **Uses standard libraries** - ESP32 WebServer + WebSocketsServer (not async alternatives)
3. **Production-ready** - All core functionality complete and tested
4. **Dual interfaces** - Both serial and web control work simultaneously
5. **Phase renumbering** - WebInterface correctly documented as Phase 5

This documentation update ensures anyone reading the project docs understands the current state: a production-ready system with comprehensive motion control accessible via both serial commands and web interface."

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
