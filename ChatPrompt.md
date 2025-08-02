I'm working on the SkullStepperV4 project - an ESP32-S3 based closed-loop stepper motor control system with DMX capability. Before we begin any coding work, please:

1. **CRITICAL: Read the README.md file first** - This contains the complete system architecture, development rules, and current project status. The README.md is the single source of truth for this project.

2. **Review Development Rules** - Pay special attention to the "Development Interaction Rules" section which includes:
   - ALWAYS reference design docs before making changes
   - NEVER remove functionality without explicit permission
   - ALWAYS update design docs when implementing features
   - Maintain design doc currency

3. **Understand the Architecture**:
   - Dual-core ESP32-S3 design (Core 0: real-time, Core 1: communication)
   - Complete module isolation (no direct inter-module calls)
   - Thread-safe communication via FreeRTOS queues
   - All modules communicate through GlobalInfrastructure

4. **Current Status (v4.1.0 - 2025-02-02)**: 
   - **Production-Ready System** with full web interface integration
   - Phase 1-5 COMPLETE: Hardware, Config, Serial, Motion Control, Web Interface
   - System features industrial-grade motion control with dual interfaces
   - Ready for Phase 6 (DMXReceiver) implementation

5. **Completed Features**:
   - **Motion Control**: ODStepper integration with auto-range homing
   - **Safety**: Hardware limits with fault latching, ESTOP behavior
   - **Serial Interface**: Complete command system with JSON API
   - **Web Interface**: Real-time control at 192.168.4.1 with WebSockets
   - **Configuration**: Persistent storage in ESP32 flash
   - **Testing**: Built-in TEST and TEST2 commands (serial and web)

6. **Key Technical Details**:
   - CL57Y closed-loop stepper driver on GPIO 7/15/16/8
   - Limit switches on GPIO 17/18 with noise filtering
   - WiFi AP mode: "SkullStepper" network
   - WebSocket updates at 10Hz on port 81
   - Thread-safe operation with FreeRTOS

7. **Before Writing Any Code**:
   - Ask me what we're working on today
   - Verify which module/feature we're implementing
   - Confirm it aligns with the documented architecture
   - Ensure we're not duplicating existing functionality

The project follows strict modular design principles with thread-safe operation. Each module must be completely isolated and communicate only through defined interfaces. This is a production-quality system, not a prototype.

Please confirm you've reviewed the design documentation and understand the development rules before we proceed with any implementation work.