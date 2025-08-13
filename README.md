# SkullStepperV4 - ESP32-S3 Closed-Loop Stepper Control System

**Version**: 4.1.13  
**Date**: 2025-02-08  
**Status**: Production-Ready with DMX Development in Progress

## System Architecture Overview

### Core Design Principles

1. **Complete Module Isolation**: Modules NEVER directly call functions from other modules
2. **Thread-Safe Communication**: All inter-module communication through FreeRTOS queues and protected data structures
3. **Dual-Core Architecture**: ESP32-S3's two cores are used for specific responsibilities
4. **Memory Safety**: No shared pointers, only value copying through protected interfaces

### Core Assignment and Responsibilities

#### Core 0 - Real-Time Operations
- **Priority**: Time-critical operations with minimal latency
- **Modules**:
  - StepperController (motion control)
  - SafetyMonitor (future - limit switches, alarms)
  - DMXReceiver (future - DMX packet processing)
- **Characteristics**: 
  - Minimal serial output to avoid timing disruption
  - Hardware interrupt handling
  - Direct hardware control

#### Core 1 - Communication and Configuration
- **Priority**: User interaction and system management
- **Modules**:
  - SerialInterface (command processing)
  - SystemConfig (parameter management)
  - Main Arduino loop
- **Characteristics**:
  - Heavy serial I/O
  - JSON processing
  - Flash storage operations

### Module Architecture

#### 1. GlobalInfrastructure (Foundation Layer)
**Role**: Provides thread-safe infrastructure for all other modules

**Responsibilities**:
- Initialize and manage FreeRTOS synchronization objects (mutexes, queues)
- Provide thread-safe access to global data structures
- Ensure memory safety across cores

**Key Components**:
```cpp
// Thread-safe data structures
SystemStatus g_systemStatus;  // Protected by g_statusMutex
SystemConfig g_systemConfig;  // Protected by g_configMutex

// Inter-module communication queues
QueueHandle_t g_motionCommandQueue;  // SerialInterface → StepperController
QueueHandle_t g_statusUpdateQueue;   // All modules → Monitoring
QueueHandle_t g_dmxDataQueue;       // DMXReceiver → StepperController
```

**Communication**: None - this is the foundation layer

#### 2. SystemConfig (Core 1)
**Role**: Persistent configuration management

**Responsibilities**:
- Store/retrieve configuration from ESP32 flash (Preferences library)
- Validate parameter ranges
- Provide atomic configuration updates
- Handle factory resets

**Communication**:
- **Receives**: Configuration commands via direct function calls from SerialInterface
- **Provides**: Configuration data via getConfig() - returns pointer to protected structure
- **Protection**: All access through g_configMutex

#### 3. SerialInterface (Core 1)
**Role**: Human and machine interface layer

**Responsibilities**:
- Parse human-readable commands and JSON API
- Format responses (human-readable or JSON)
- Queue motion commands for StepperController
- Manage interactive features (echo, verbosity, streaming)

**Communication**:
- **Sends**: Motion commands via g_motionCommandQueue
- **Reads**: System status via thread-safe macros
- **Calls**: SystemConfig functions for configuration changes

**Key Design**:
```cpp
// Motion command queuing
MotionCommand cmd = createMotionCommand(CommandType::MOVE_ABSOLUTE, position);
xQueueSend(g_motionCommandQueue, &cmd, timeout);
```

#### 4. StepperController (Core 0)
**Role**: Real-time motion control

**Responsibilities**:
- Process motion commands from queue
- Generate step pulses via ODStepper/FastAccelStepper
- Monitor limit switches with debouncing
- Execute homing sequences
- Enforce position limits and safety

**Communication**:
- **Receives**: Motion commands via g_motionCommandQueue
- **Updates**: System status via thread-safe macros
- **Signals**: Safety states through g_systemStatus

**Task Structure**:
```cpp
void stepperControllerTask(void* parameter) {
    // Runs every 2ms on Core 0
    while (true) {
        checkLimitSwitches();        // GPIO monitoring
        processMotionCommands();     // Queue processing
        updateHomingSequence();      // State machine
        updateMotionStatus();        // Status updates
        vTaskDelayUntil(...);        // Precise timing
    }
}
```

#### 5. SafetyMonitor (Core 0 - Future)
**Role**: Centralized safety monitoring

**Planned Responsibilities**:
- Monitor all safety-critical inputs
- Trigger emergency stops
- Manage fault conditions
- Coordinate safety responses

#### 6. DMXReceiver (Core 0 - Future)
**Role**: DMX512 protocol handler

**Planned Responsibilities**:
- Receive DMX packets via UART2
- Validate packet integrity
- Extract channel data
- Queue position updates

### Inter-Process Communication

#### 1. Motion Command Queue
**Purpose**: Send motion commands from Core 1 to Core 0
```cpp
// Structure
typedef struct {
    CommandType type;        // MOVE_ABSOLUTE, HOME, STOP, etc.
    MotionProfile profile;   // Speed, acceleration, target
    uint32_t timestamp;
    uint16_t commandId;
} MotionCommand;

// Usage
SerialInterface (Core 1) → g_motionCommandQueue → StepperController (Core 0)
```

#### 2. Thread-Safe Status Access
**Purpose**: Safe status reading/writing across cores
```cpp
// Write (any module)
SAFE_WRITE_STATUS(currentPosition, 1000);
// Expands to:
// xSemaphoreTake(g_statusMutex, timeout);
// g_systemStatus.currentPosition = 1000;
// xSemaphoreGive(g_statusMutex);

// Read (any module)
int32_t pos;
SAFE_READ_STATUS(currentPosition, pos);
```

#### 3. Configuration Access
**Purpose**: Thread-safe configuration management
```cpp
// Get config pointer (read-only access)
SystemConfig* config = SystemConfigMgr::getConfig();

// Update configuration
SystemConfigMgr::setMotionProfile(profile);
SystemConfigMgr::commitChanges();  // Saves to flash
```

### Communication Rules

1. **No Direct Module Calls**: Modules never include headers or call functions from other modules (except GlobalInterface.h)

2. **Queue-Based Commands**: All commands flow through FreeRTOS queues
   - Non-blocking sends (timeout = 0)
   - Prevents deadlocks
   - Allows command rejection when full

3. **Protected Shared Data**: All shared data access through mutexes
   - Short critical sections
   - Consistent timeout handling
   - No mutex holding across function calls

4. **Value Semantics**: Data is copied, never shared by reference
   - Prevents race conditions
   - Ensures data consistency
   - Allows independent module updates

### Adding New Functionality

When adding new modules or features:

1. **Determine Core Assignment**:
   - Real-time/hardware control → Core 0
   - User interface/configuration → Core 1

2. **Define Communication**:
   - Create new queue if needed
   - Add to GlobalInfrastructure
   - Define message structures

3. **Respect Module Boundaries**:
   - Only include GlobalInterface.h
   - Never call other modules directly
   - Use queues for commands
   - Use protected status for state

4. **Maintain Thread Safety**:
   - Always use provided macros
   - Keep critical sections short
   - Test with concurrent operations

### Example: Adding a New Feature

To add LED status indicators:

1. **Create LED Controller Module** (Core 0 or 1 depending on timing needs)
2. **Add Status Fields** to SystemStatus for LED states
3. **Create Command Queue** if LED patterns need commands
4. **Update GlobalInfrastructure** with new queue initialization
5. **Have modules update LED status** via SAFE_WRITE_STATUS
6. **LED Controller reads status** and updates physical LEDs

This architecture ensures scalability while maintaining thread safety and module independence.

## System Overview

This is a comprehensive modular stepper motor control system with DMX input and extensive configurability, designed for the ESP32-S3 microcontroller with **CL57Y closed-loop stepper driver**.

## Build and Compilation

### Hardware Requirements
- **Board**: ESP32-S3-DevKitC-1 or compatible ESP32-S3 development board
- **Memory**: 8MB Flash recommended
- **Connection**: USB-C for programming and serial communication

### Arduino CLI Compilation

#### Prerequisites
1. Install Arduino CLI: `brew install arduino-cli` (macOS) or download from [arduino.cc](https://arduino.github.io/arduino-cli/)
2. Install ESP32 board support:
```bash
arduino-cli core install esp32:esp32
```

#### Compile Command
```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,PartitionScheme=default_8MB,CPUFreq=240,FlashMode=qio,FlashSize=8M,UploadSpeed=921600,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,USBMode=hwcdc /path/to/SkullStepperV4 --warnings all
```

#### Upload Command (when board is connected)
```bash
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,PartitionScheme=default_8MB,CPUFreq=240,FlashMode=qio,FlashSize=8M,UploadSpeed=921600,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,USBMode=hwcdc /path/to/SkullStepperV4
```

#### Board Configuration Parameters
- **FQBN**: `esp32:esp32:esp32s3` (Vendor:Architecture:Board)
- **CDC on Boot**: Enabled (for USB serial communication)
- **Partition Scheme**: Default 8MB
- **CPU Frequency**: 240MHz
- **Flash Mode**: QIO (Quad I/O)
- **Flash Size**: 8MB
- **Upload Speed**: 921600 baud
- **PSRAM**: OPI (Octal SPI)
- **USB Mode**: Hardware CDC

#### Memory Usage (v4.1.13)
- **Program Storage**: ~1.2MB (34% of 3.3MB available)
- **Global Variables**: ~49KB (15% of 320KB available)
- **Free Heap**: ~278KB available for runtime

## Development Status

**Current Phase: Production-Ready System with Full Web Interface Integration**

### 🎯 Major Milestone Achieved
The system is now **production-ready** with a complete web-based control interface alongside the comprehensive serial command system. All core functionality is implemented, tested, and documented.

### ✅ Phase 1: Foundation (Complete - Hardware Tested)
- [x] HardwareConfig.h - Pin assignments and hardware constants
- [x] GlobalInterface.h - Data structures, enums, and interfaces  
- [x] Main Arduino sketch - Basic setup/loop with module framework
- [x] Module header files - Interface definitions for all modules
- [x] Module stub implementations - Compilation framework
- [x] **Hardware Testing** - ESP32-S3 pin configuration and I/O verification
- [x] **Open-drain Configuration** - Proper CL57Y stepper driver compatibility
- [x] **FreeRTOS Integration** - Dual-core task framework operational
- [x] **Serial Diagnostics** - Hardware test suite and status monitoring

### ✅ Phase 2: SystemConfig Module (Complete - Flash Storage)
- [x] **Flash-based Configuration** - ESP32 Preferences library implementation
- [x] **Parameter Persistence** - All config values survive reboots
- [x] **Memory Corruption Resolution** - Eliminated EEPROM/RAM conflicts
- [x] **Wear Leveling Protection** - ESP32 flash optimization
- [x] **Atomic Configuration Updates** - Power-safe storage operations
- [x] **Comprehensive Parameter Coverage** - Motion, Position, DMX, Safety, System settings
- [x] **JSON Import/Export** - Configuration serialization capabilities
- [x] **Parameter Validation** - Range checking and integrity verification
- [x] **Factory Reset Functionality** - Return to default configuration
- [x] **Thread-safe Access** - FreeRTOS mutex protection

### ✅ Phase 3: SerialInterface Module (Complete - Interactive Command Processing)
- [x] **Human-Readable Commands** - Interactive command line interface with prompt
- [x] **JSON API** - Complete JSON command and response system
- [x] **Real-time Configuration** - Live parameter updates with validation
- [x] **Motion Command Integration** - Queue-based motion control commands
- [x] **Status Reporting** - Comprehensive system status in human and JSON formats
- [x] **Output Mode Control** - Switchable human/JSON output modes
- [x] **Interface Controls** - Echo, verbosity, streaming controls
- [x] **Parameter Reset Commands** - Individual and factory reset capabilities
- [x] **Comprehensive Help System** - Interactive command documentation
- [x] **Error Handling** - Detailed validation and error reporting
- [x] **Thread-Safe Operation** - Core 1 execution with proper synchronization
- [x] **DMX Configuration Interface** - Complete DMX parameter control
- [x] **Enhanced JSON Config Output** - Metadata with ranges, units, and validation info

### ✅ Phase 4: StepperController Module (COMPLETE - Production Quality)
- [x] **Thread-Safe Infrastructure** - GlobalInfrastructure.cpp with FreeRTOS mutexes/queues
- [x] **ODStepper Integration** - Successfully integrated with FastAccelStepper backend
- [x] **Motion Profile Management** - Thread-safe speed/acceleration control
- [x] **Dynamic Target Updates** - Seamless position changes while moving (ready for DMX)
- [x] **Software Position Limits** - Min/max position enforcement
- [x] **Hardware Limit Switches** - LEFT_LIMIT (GPIO 17) and RIGHT_LIMIT (GPIO 18) fully functional
- [x] **Auto-Range Homing Routine** - Detects physical travel limits automatically
- [x] **Integration with SerialInterface** - All motion commands working
- [x] **CL57Y ALARM Monitoring** - Position following error detection on GPIO 8
- [x] **Enhanced Noise Filtering** - 3-sample validation + 100ms debounce
- [x] **Continuous Limit Monitoring** - Limits reliably stop motion with proper fault latching
- [x] **Industrial Safety Standards** - Limit faults require homing to clear (proper ESTOP behavior)
- [x] **Position Holding** - Motor stays enabled by default
- [x] **Test Commands** - PARAMS, TEST (range test), and TEST2 (random test)
- [x] **Configuration Persistence** - All motion parameters load from flash on boot
- [x] **User-Configurable Homing Speed** - Adjustable via CONFIG command

### ✅ Phase 5: WebInterface Module (COMPLETE - Core System Component)
- [x] **WiFi Access Point** - Standalone network "SkullStepper" (no router required)
- [x] **Built-in Web Server** - ESP32 WebServer on port 80
- [x] **WebSocket Server** - Real-time bidirectional communication on port 81
- [x] **Responsive Web UI** - Mobile-friendly interface with touch controls
- [x] **Real-Time Status Updates** - 10Hz WebSocket updates for smooth UI
- [x] **Complete Motion Control** - Move, jog, home, stop, emergency stop
- [x] **Test Functions** - Web-based TEST and TEST2 buttons for system validation
- [x] **Live Configuration** - Real-time parameter adjustment with immediate effect
- [x] **Safety Integration** - Proper homing requirements and limit fault handling
- [x] **Status Dashboard** - Position, speed, limits, and system state visualization
- [x] **Captive Portal** - Automatic redirect when connecting to WiFi
- [x] **REST API** - HTTP endpoints for automation integration
- [x] **Thread-Safe Design** - Proper integration with existing architecture
- [x] **Zero External Dependencies** - All HTML/CSS/JS embedded in firmware

### 🚧 Phase 6: DMXReceiver Module (IN PROGRESS - 2025-02-02)
- [x] **Phase 1 Complete**: Core DMX Infrastructure with ESP32S3DMX
  - ESP32S3DMX library integration on UART2/GPIO4
  - Core 0 real-time task for DMX processing
  - 5-channel cache system with improved layout
  - Signal loss detection and timeout handling
  - Thread-safe status updates
- [x] **Phase 2 Complete**: Channel Processing
  - Position mapping (8-bit and 16-bit modes)
  - Speed/acceleration scaling from DMX values  
  - Mode detection with hysteresis
- [x] **Phase 3 Complete**: System Integration
  - Full StepperController integration
  - Dynamic parameter application
  - Safety checks and mode transitions
- [x] **Phase 4 Complete**: Motion Integration
  - DMX controls position, speed, and acceleration
  - Smooth mode transitions (Stop/Control/Home)
  - Signal loss position hold
- [ ] **Phase 5**: Web Interface Updates (DMX status display, configuration)
- [ ] **Phase 6**: Serial Interface Updates (DMX commands and monitoring)
- [ ] **Phase 7**: 16-bit Position Implementation UI
- [ ] **Phase 8**: Testing & Validation

### 🔄 Future Enhancement Phases
- [ ] **Phase 7:** SafetyMonitor Module (Centralized safety monitoring)
- [ ] **Phase 8:** Advanced Features (Position persistence, data logging, etc.)

## Design Documentation

### Available Documentation:
- **[Quick Reference](QUICK_REFERENCE.md)** - Essential commands and operations
- **[SerialInterface Manual](SerialInterface_Manual.md)** - Complete command reference and API documentation
- **[WebInterface Design](WebInterface_Design.md)** - Web interface design specification
- **[WebInterface Functionality Status](webfuncstat.md)** - Feature comparison matrix
- **[Design Documentation](design_docs.md)** - Detailed architecture and implementation
- **[Testing Protocol](TESTING_PROTOCOL.md)** - Comprehensive testing procedures
- **[Publishing Guide](PUBLISHING.md)** - Project management and git workflow
- **[Changelog](CHANGELOG.md)** - Version history and changes

## Development Interaction Rules

### **CRITICAL DEVELOPMENT STANDARDS** ⚠️

1. **ALWAYS Reference Design Docs First**
   - Read README.md and all design documentation before making ANY changes
   - Understand current phase status and implemented features
   - Verify architecture and modular design principles
   - Respect existing functionality and interfaces

2. **NEVER Remove Functionality Without Permission**
   - Ask explicitly before removing ANY features or capabilities
   - Preserve all existing functionality by default
   - Only enhance existing systems, don't replace them
   - Get approval for any breaking changes

3. **ALWAYS Update Design Docs When Making Changes**
   - Update README.md with new phase completions and status
   - Document new features and capabilities as implemented
   - Update module status and current implementation state
   - Keep design docs as the **single source of truth**

4. **Maintain Design Doc Currency**
   - Anyone should be able to read docs and understand current state
   - All implemented features must be documented
   - Clear roadmap for what's next and what's in progress
   - Enable fresh start from documented state at any time

**These rules ensure professional development practices and prevent loss of implemented functionality.**

## Closed-Loop Stepper Design

### **CL57Y Closed-Loop Driver Features**
- **Built-in Encoder Feedback** - Internal position control and step verification
- **Automatic Error Correction** - Driver compensates for missed steps automatically
- **ALARM Output** - Signals when motor cannot follow commanded position
- **High Reliability** - Maintains accuracy under varying loads
- **No External Feedback Required** - Self-contained position control

### **Control Strategy**
```
Controller → Step/Dir Pulses → CL57Y → Motor with Encoder
     ↑                           ↓
Position Tracking          ALARM Signal
(Commanded Steps)         (Following Error)
```

### **Feedback Architecture**
- **Position Tracking**: Count of step pulses COMMANDED to CL57Y
- **Error Detection**: CL57Y ALARM pin indicates following problems
- **No Encoder Reading**: CL57Y handles internal position control
- **Simplified Control**: No complex feedback compensation required

## Hardware Configuration

### **CL57Y Closed-Loop Stepper Control:**
- **STEP**: GPIO 7 → CL57Y PP+ (Open-drain + 1.8kΩ to +5V) - Handled by ODStepper
- **DIR**: GPIO 15 → CL57Y DIR+ (Open-drain + 1.8kΩ to +5V) - Handled by ODStepper
- **ENABLE**: GPIO 16 → CL57Y MF+ (Open-drain + 1.8kΩ to +5V) - HIGH = Enabled - Handled by ODStepper
- **ALARM**: GPIO 8 ← CL57Y ALARM+ (Input with pull-up) - Position following error

### **Motion Control with ODStepper**
- **Library**: ODStepper (wrapper for FastAccelStepper with automatic open-drain)
- **Pulse Generation**: Hardware timer-based via FastAccelStepper
- **Motion Profiles**: Trapezoidal acceleration/deceleration
- **Maximum Frequency**: Up to 200kHz on ESP32
- **Dynamic Updates**: Seamless target changes while moving (perfect for DMX)
- **High-Speed Support**: 500mm/s with 20-tooth GT3 (3333 steps/s)
- **Interrupt Safety**: Minimal ISRs to avoid timing conflicts

### **ESP32-S3 Pin Assignments**
**DMX Interface (MAX485):**
- RO: GPIO 4 (UART2 RX) ✅ Initialized
- DI: GPIO 6 (UART2 TX) ✅ Initialized
- DE/RE: GPIO 5 (Direction control) ✅ Configured

**Limit Switches:**
- LEFT_LIMIT: GPIO 17 (Active low with pull-up) ✅ Tested
- RIGHT_LIMIT: GPIO 18 (Active low with pull-up) ✅ Tested

## Key Design Principles

### **Complete Module Isolation**
- **Zero Direct Dependencies**: Modules NEVER call functions from other modules
- **Interface-Only Communication**: All inter-module communication through defined interfaces
- **Independent Development**: Update any module without touching others
- **Compile-Time Isolation**: Each module can be disabled via #define

### **Closed-Loop Motion Control**
- **Commanded Position Tracking**: Count step pulses sent to CL57Y
- **Hardware Feedback**: CL57Y ALARM monitoring for error detection
- **Simplified Control Logic**: No complex feedback compensation required
- **High Reliability**: CL57Y handles step loss recovery automatically

### **Motion Control with ODStepper (Phase 4)**
- **Trapezoidal Profiles**: Smooth acceleration/deceleration ramps
- **Hardware Timer-Based**: Precise pulse generation via FastAccelStepper
- **Dynamic Target Updates**: Seamless position changes while moving
- **Professional Quality**: Eliminates stepping artifacts with smooth motion
- **Open-Drain Automatic**: ODStepper configures pins for CL57Y compatibility

### **Thread Safety & Memory Safety**
- **Dual-Core Architecture**: 
  - **Core 0**: Real-time operations (StepperController, DMXReceiver, SafetyMonitor)
  - **Core 1**: Communication/monitoring (SerialInterface, SystemConfig)
- **FreeRTOS Protection**: Mutexes, queues, and atomic operations
- **Memory Protection**: No shared pointers, only value copying
- **Race Condition Prevention**: All shared data access protected

### **Flash Storage Architecture (Phase 2)**
- **ESP32 Preferences Library**: Native key-value flash storage
- **Wear Leveling**: Automatic flash longevity management
- **Atomic Operations**: Power-safe configuration updates
- **Type-Safe Storage**: Individual parameter validation and storage
- **Namespace Isolation**: "skullstepper" partition prevents conflicts

### **Interactive Command Interface (Phase 3)**
- **Dual Interface Support**: Human-readable commands and JSON API
- **Real-time Configuration**: Live parameter updates with immediate validation
- **Comprehensive Command Set**: Motion control, configuration, status, diagnostics
- **Output Mode Control**: Switchable between human and JSON response formats
- **Advanced Reset Capabilities**: Individual parameter, group, and factory reset

## Module Overview

### ✅ GlobalInfrastructure (Core 0/1 - Complete - Thread Safety Foundation)
**Implemented Features:**
- **FreeRTOS Mutexes**: Thread-safe data access protection
- **Inter-Module Queues**: Motion commands, status updates, DMX data
- **Memory Safety**: Protected global data structures
- **System State Management**: Thread-safe state transitions
- **Integrity Validation**: System health monitoring
- **Infrastructure Diagnostics**: Complete status reporting

### ✅ SystemConfig (Core 1 - Complete)
**Implemented Features:**
- **Flash Storage**: ESP32 Preferences-based configuration persistence
- **Parameter Management**: Motion profiles, position limits, DMX settings, safety configuration
- **Data Validation**: Comprehensive range checking and integrity verification
- **JSON Serialization**: Import/export configuration as JSON with metadata
- **Factory Reset**: Return to safe default values
- **Thread-Safe Access**: FreeRTOS mutex protection for all operations
- **Wear Leveling**: Automatic flash longevity protection
- **Atomic Updates**: Power-safe configuration changes

### ✅ SerialInterface (Core 1 - Complete)
**Implemented Features:**
- **Human Command Interface**: Interactive command line with prompt (`skull> `)
- **JSON API**: Complete JSON command and response system
- **Motion Commands**: MOVE, HOME, STOP, ESTOP, ENABLE, DISABLE
- **Configuration Commands**: CONFIG SET/RESET with comprehensive parameter support
- **Interface Controls**: ECHO, VERBOSE, JSON, STREAM mode controls
- **Status Reporting**: Real-time system status in multiple formats
- **Reset Capabilities**: Individual parameter, group, and factory reset
- **Help System**: Comprehensive interactive documentation
- **Output Modes**: Switchable human-readable and JSON output
- **Error Handling**: Detailed validation and clear error messages

### ✅ WebInterface (Core 1 - Complete) - Phase 5
**Implemented Features:**
- **WiFi Access Point**: Direct connection without router (SSID: "SkullStepper")
- **Built-in Web Server**: ESP32 WebServer on port 80
- **WebSocket Support**: Real-time bidirectional communication at 10Hz on port 81
- **Responsive Web UI**: Mobile-friendly interface with touch controls
- **Motion Control**: Move, jog, home, stop, emergency stop buttons
- **Live Configuration**: Sliders for speed and acceleration adjustment
- **Status Dashboard**: Real-time position, speed, and limit indicators
- **System Information Display**: Version, uptime, memory stats, task monitoring
- **REST API**: HTTP endpoints for automation integration
- **Thread-Safe Design**: Uses existing queues and protected data access
- **Embedded Assets**: All HTML/CSS/JS stored in PROGMEM (no filesystem needed)
- **Connection Management**: Supports up to 2 simultaneous WebSocket clients

### ✅ StepperController (Core 0 - Real-Time) - PHASE 4 COMPLETE
**Implemented Features:**
- **ODStepper Integration**: Thread-safe wrapper with FastAccelStepper backend
- **Motion Profiles**: Smooth trapezoidal acceleration/deceleration
- **Dynamic Target Updates**: Seamless position changes while moving (ready for DMX)
- **Position Limits**: Software min/max enforcement with safety checks
- **Hardware Limit Switches**: LEFT_LIMIT (GPIO 17) and RIGHT_LIMIT (GPIO 18) fully functional
- **Auto-Range Homing**: Automatically detects physical travel limits
- **Thread-Safe Operation**: Dedicated Core 0 task with FreeRTOS
- **Motion Command Queue**: Processes commands from SerialInterface and WebInterface

- **CL57Y ALARM Monitoring**: Position following error detection on GPIO 8
- **Industrial Safety Standards**: Limit faults require homing to clear
- **Position Holding**: Motor stays enabled by default
- **Test Commands**: PARAMS, TEST (range test), and TEST2 (random test)
- **Configuration Persistence**: All motion parameters load from flash on boot
- **User-Configurable Homing Speed**: Adjustable via CONFIG command or web interface

### 🚧 DMXReceiver (Core 0 - Real-Time) - PHASE 6 IN PROGRESS
**Phase 1 Implemented (2025-02-02):**
- ESP32S3DMX library integration for DMX512 reception on UART2
- Core 0 real-time task with 10ms update cycle
- 5-channel operation with configurable base offset (1-508)
- Channel layout: Position MSB, Position LSB, Acceleration, Speed, Mode
- Signal presence detection and timeout handling (configurable 100-60000ms)
- Thread-safe communication with system status updates
- Packet statistics tracking (total/error counts)

**Remaining Implementation:**
- Channel value processing and mode detection
- Position mapping (8-bit and 16-bit modes)
- Dynamic speed/acceleration scaling
- Integration with StepperController for motion commands
- Web and serial interface integration

### SafetyMonitor (Core 0 - Real-Time) - PLANNED
- Monitor limit switch states with debouncing
- Monitor CL57Y ALARM signal for position following errors
- Enforce software position boundaries
- Trigger safe stop sequences on faults

## Configuration Parameters (Complete Implementation)

### **Motion Profile Parameters**

| Parameter | Range | Units | Default | Description |
|-----------|-------|-------|---------|-------------|
| `maxSpeed` | 0-10000 | steps/sec | 1000 | Maximum velocity |
| `acceleration` | 0-20000 | steps/sec² | 500 | Acceleration rate |
| `deceleration` | 0-20000 | steps/sec² | 500 | Deceleration rate |
| `jerk` | 0-50000 | steps/sec³ | 1000 | Jerk limitation |
| `homingSpeed` | 0-10000 | steps/sec | 940 | Speed during homing |

**Note**: FastAccelStepper uses same value for acceleration/deceleration

### **Mechanical Configuration** (20-tooth GT3 pulley)
| Linear Speed | Step Rate | Use Case |
|--------------|-----------|----------|
| 15 mm/s | 100 steps/s | Homing speed |
| 75 mm/s | 500 steps/s | Slow positioning |
| 150 mm/s | 1000 steps/s | Default max speed |
| 300 mm/s | 2000 steps/s | Fast positioning |
| 500 mm/s | 3333 steps/s | High-speed operation |

### **Position Control**
- **homePosition**: steps (Reference point)
- **minPosition**: steps (Software minimum)
- **maxPosition**: steps (Software maximum)
- **range**: minimum 100 steps

### **DMX Configuration**
- **dmxStartChannel**: 1-512 (DMX channel to monitor)
- **dmxScale**: steps/DMX_unit (Position scaling, non-zero)
- **dmxOffset**: steps (Position offset)
- **dmxTimeout**: 100-60000 ms (Signal timeout)

### **Safety Settings**
- **enableLimitSwitches**: boolean (Monitor limit switches)
- **enableStepperAlarm**: boolean (Monitor CL57Y ALARM signal)
- **emergencyDeceleration**: 100-50000 steps/sec² (Emergency stop rate)

## System Features Summary

### 🌐 Control Interfaces
1. **Serial Command Interface**
   - Human-readable commands with interactive prompt
   - Complete JSON API for automation
   - Comprehensive help system
   - Real-time status monitoring

2. **Web Control Interface** 
   - WiFi Access Point: "SkullStepper"
   - Browse to: http://192.168.4.1
   - Real-time WebSocket updates at 10Hz
   - Mobile-responsive design
   - Complete motion and configuration control

### 🔒 Safety Features
1. **Hardware Limit Protection**
   - Continuous monitoring with interrupt backup
   - Industrial-standard fault latching
   - Emergency stop on unexpected limit activation
   - Requires homing to clear faults

2. **Software Safety**
   - Position limits enforced
   - Homing required before movement
   - CL57Y ALARM monitoring
   - Thread-safe operation across dual cores

### 🎯 Motion Control
1. **Professional Motion Profiles**
   - Hardware timer-based step generation
   - Smooth trapezoidal acceleration
   - Dynamic target updates (DMX ready)
   - Up to 10kHz step rates

2. **Auto-Range Homing**
   - Automatically detects physical travel limits
   - Sets safe operating boundaries
   - No manual limit configuration needed
   - Configurable homing speed

### 💾 Configuration Management
1. **Persistent Storage**
   - All settings saved to ESP32 flash
   - Survives power cycles
   - Factory reset capability
   - Parameter validation

2. **Live Configuration**
   - Change parameters without restart
   - Immediate effect on motion
   - Web and serial interfaces
   - Comprehensive parameter documentation

## Next Steps for Project Enhancement

### Immediate Production Deployment:
1. **Hardware Hardening**
   - Install 1kΩ + 0.1µF RC filters on limit switches
   - Use shielded cables for limit switches
   - Add ferrite beads for EMI suppression
   - Consider external emergency stop button

2. **System Validation**
   - Run extended TEST sequences
   - Verify limit protection under all conditions
   - Tune speed/acceleration for your mechanics
   - Test web interface on various devices

### Phase 6 - DMXReceiver Module (IN PROGRESS):
**Completed (Phase 1 - 2025-02-02):**
- ✅ ESP32S3DMX library integration on UART2/GPIO4
- ✅ Core 0 real-time task for DMX processing
- ✅ 5-channel cache with improved layout (Position MSB/LSB adjacent)
- ✅ Signal detection and timeout handling
- ✅ Base channel configuration (1-508)
- ✅ Thread-safe status updates

**Remaining (Phases 2-8):**
- Implement channel value processing and mode detection
- Add position mapping (8-bit and 16-bit modes)
- Scale DMX values to position/speed/acceleration
- Queue position updates to StepperController
- Web interface DMX status display and configuration
- Serial commands for DMX monitoring and control
- Complete testing with DMX consoles

### Phase 7 - SafetyMonitor Module (Optional Refactor):
- Most safety features already implemented in StepperController
- Could centralize all safety monitoring
- Add watchdog timer for motion timeouts
- Enhanced diagnostics and logging
- Predictive maintenance features

### Phase 8 - Advanced Features:
1. **Position Persistence**
   - Save position to flash periodically
   - Restore position on power-up
   - Optional absolute encoder support

2. **Data Logging**
   - Motion history recording
   - Error event logging
   - Performance metrics
   - Web-based log viewer

3. **Advanced Web Features**
   - Multi-axis visualization
   - Motion profile editor
   - Backup/restore configuration
   - Firmware OTA updates

### Known Limitations:
1. **No Automatic Recovery** - Limit faults require manual homing (by design)
2. **Single Axis** - System controls one stepper motor
3. **No Position Feedback** - Relies on CL57Y internal encoder

## Current Status Summary

✅ **Phase 1**: Hardware foundation and module framework - COMPLETE  
✅ **Phase 2**: Configuration management with flash storage - COMPLETE  
✅ **Phase 3**: Interactive command interface (human & JSON) - COMPLETE  
✅ **Phase 4**: Motion control with ODStepper integration - COMPLETE  
✅ **Phase 5**: WebInterface module - COMPLETE (WiFi control interface)
🚧 **Phase 6**: DMXReceiver module - IN PROGRESS (Phase 1 of 8 complete)
🔄 **Phase 7**: SafetyMonitor module - OPTIONAL (safety already integrated)
🔄 **Phase 8**: Advanced features - FUTURE

**Current State (2025-02-02)**: 
- Production-ready motion control system with professional-quality features
- Auto-range homing with configurable speed
- Robust limit switch protection with industrial-standard fault latching
- Full configuration persistence across power cycles
- Comprehensive command interface supporting both human and JSON formats
- Complete web interface with real-time control and monitoring
- All motion parameters user-configurable via serial or web
- **Fixed**: Web interface stress test now runs continuously (matches serial interface)
- **New**: Configurable home position as percentage of range (survives reboot)
- **DMX Progress**: Core infrastructure implemented with ESP32S3DMX library
- Ready for production deployment!

**Recent Development (2025-01-31):**

**Configuration System Enhancements:**
- ✅ **Fixed Configuration Loading on Boot**
  - StepperController now properly loads saved configuration from flash on initialization
  - Motion parameters (maxSpeed, acceleration, etc.) persist across reboots
  - Added configuration loading diagnostics in startup sequence
  - Ensures user settings are always applied on system startup

- ✅ **Added User-Configurable Homing Speed**
  - Homing speed is now a configurable parameter (was hardcoded)
  - Range: 0-10000 steps/sec, Default: 940 steps/sec
  - Accessible via `CONFIG SET homingSpeed <value>` command
  - Persists across reboots with other configuration parameters
  - Included in CONFIG, PARAMS, and JSON outputs
  - Can be reset individually or with motion parameter group

- ✅ **Fixed JSON Command Processing**
  - JSON config command now works with just `{"command":"config"}`
  - No longer requires `"get":"all"` syntax
  - JSON commands work best when not in JSON output mode
  - Added clear documentation about JSON command usage

**Major Accomplishments:**
- ✅ **Fixed Intermittent Limit Switch Detection**
  - Implemented continuous monitoring with redundant detection (interrupt + polling)
  - Added enhanced debounce logic with 3-sample validation
  - Flags now cleared only after state confirmation
  - Optimized for minimal CPU impact (only 0.2% overhead)
  - Clear status messages for both activation and release events

- ✅ **Improved Motion Control**
  - Reduced homing speed by 50% (now 375 steps/sec) for safer operation
  - Fixed CONFIG SET commands not applying to StepperController
  - Motion parameters now update immediately when changed
  - Resolved acceleration/deceleration jerk issue (mechanical - loose coupler)

- ✅ **Enhanced Safety Features**
  - Implemented proper limit fault latching system
  - Motion commands rejected when limit fault is active
  - Requires homing sequence to clear faults (industrial standard)
  - Fixed TEST command to stop cleanly on limit fault
  - Prevents command queue flooding during fault conditions

- ✅ **Added Diagnostic Capabilities**
  - Implemented step timing diagnostics (DIAG ON/OFF command)
  - Captured step intervals to identify mechanical issues
  - Successfully diagnosed loose coupler causing motion jerks
  - Removed diagnostic overhead after troubleshooting

- ✅ **New Test Routines**
  - TEST2/RANDOMTEST command - moves to 10 random positions
  - Helps verify smooth motion across full range
  - Both tests interruptible with any keypress
  - Shows test progress and completion status

**Key Lessons Learned:**
- Mechanical issues (loose coupler) can manifest as software timing problems
- Step interval diagnostics are valuable for troubleshooting
- Continuous limit monitoring at 2ms provides good balance of safety and performance
- Proper fault latching prevents dangerous repeated limit hits

**Recent Development (2025-01-30):**
- ✅ Successfully integrated ODStepper/FastAccelStepper for smooth motion control
- ✅ Implemented complete auto-range homing sequence that detects physical limits
- ✅ Fixed critical limit switch bug - now properly stops motion during operation
- ✅ Added enhanced noise filtering with 3-sample validation
- ✅ Added PARAMS command to list all configurable parameters with ranges
- ✅ Added TEST command for automated range testing (10% to 90% of detected range)
- ✅ Motor now holds position by default (prevents unwanted movement)
- ✅ Extended homing timeout to 90 seconds for longer travel distances
- 🛠️ Recommended hardware RC filters (1kΩ + 0.1µF) for EMI immunity

## Phase 4 Implementation Details (ODStepper Integration)

### **Core Assignment - CRITICAL**
**StepperController MUST run on Core 0** (Real-time operational core)
- Core 0 handles all time-critical operations
- Direct hardware access without OS scheduling delays
- Shares core with future DMXReceiver and SafetyMonitor
- Communicates with Core 1 via thread-safe queues

### **Architecture Overview**
StepperController will be a thread-safe wrapper around ODStepper that:
1. **Runs on Core 0** for real-time performance
2. Uses ODStepper/FastAccelStepper for all motion control
3. Adds thread-safe access with dedicated mutex
4. Implements software position limits (min/max)
5. Implements hardware limit switches (GPIO 17/18)
6. Provides homing routine using limit switches
7. Processes commands from motion queue (sent by Core 1)

### **Key Design Decisions**

#### Motion Profile Management
- Motion parameters passed through to ODStepper
- Support for dynamic updates while moving
- Parameters: maxSpeed, acceleration (decel same as accel)
- Note: Trapezoidal profiles, not S-curves

#### Limit Switch Implementation
- LEFT_LIMIT: GPIO 17 (active low with pull-up)
- RIGHT_LIMIT: GPIO 18 (active low with pull-up)
- **Hybrid approach**: Minimal ISR + fast polling
- **ISR Strategy**: Ultra-minimal to avoid interfering with FastAccelStepper timing
- **Response time**: 200μs polling for high-speed operation (500mm/s capability)
- Emergency stop via flags, debouncing in main loop

#### High-Speed Considerations (500mm/s)
With 20-tooth GT3 pulley (3mm pitch):
- 500mm/s = 3,333 steps/sec
- 0.15mm per step
- 200μs polling = 0.1mm worst-case overtravel
- Interrupt flags for immediate detection
- Debouncing handled in Core 0 task to avoid timing conflicts

#### Position Limits
- Software limits from SystemConfig
- Checked before accepting moves
- Clamp DMX commands to valid range
- Emergency stop if exceeded during motion

#### Homing Sequence (Auto-Range Detection)
1. **Find Left Limit (Home)**:
   - Move slowly toward left limit (100 steps/s)
   - Stop on detection, back off slightly (50 steps)
   - Set current position as 0 (home reference)

2. **Find Right Limit (Determine Range)**:
   - Move slowly toward right limit
   - Stop on detection, back off slightly (50 steps)
   - Record this position as physical maximum

3. **Set Operating Bounds**:
   - `minPosition` = 0 + safety margin (e.g., 10 steps)
   - `maxPosition` = detected right position - safety margin (e.g., 10 steps)
   - These become the runtime position limits

4. **Return to Ready Position**:
   - Move to center of detected range or configured ready position
   - System now knows exact physical range for safe operation

**Benefits**:
- Automatically adapts to mechanical installation
- No manual configuration of position limits needed
- Prevents commands beyond physical limits
- Config min/max values become optional software limits within physical range

#### DMX Control Pattern (Future)
```cpp
// Seamless position updates while moving:
int32_t targetPos = (dmxValue * dmxScale) + dmxOffset;
targetPos = constrain(targetPos, minPosition, maxPosition);
StepperController::moveTo(targetPos); // Smooth update
```

### **Dependencies**
- ODStepper library (install via Arduino Library Manager)
- FastAccelStepper (automatically installed with ODStepper)

### **Safety Implementation Notes**

#### Interrupt Safety with FastAccelStepper
- FastAccelStepper uses hardware timers and interrupts for pulse generation
- Our limit switch ISRs must be minimal to avoid timing interference
- Solution: Single-instruction ISRs that only set volatile flags
- All debouncing and processing done in Core 0 task

#### Limit Switch Response Times
| Speed | Polling Rate | Max Overtravel | Notes |
|-------|--------------|----------------|--------|
| 150 mm/s | 200μs | 0.03mm | Default speed |
| 300 mm/s | 200μs | 0.06mm | Fast moves |
| 500 mm/s | 200μs | 0.1mm | High speed |
| Homing | 200μs | <0.01mm | Slow speed |

#### Emergency Stop Behavior
- Limit ISR sets flag only (no direct motor control)
- Core 0 task detects flag within 200μs
- Calls `stepper->stopMove()` for controlled deceleration
- Or `stepper->forceStop()` for immediate halt
- Debouncing prevents false triggers

### **Core 0 Task Structure**
```cpp
// Core 0 real-time task (created during initialization)
void stepperControllerTask(void* parameter) {
    const TickType_t xFrequency = pdUS_TO_TICKS(200); // 200μs for high-speed
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (true) {
        // Check limit switch flags from ISR (minimal overhead)
        if (leftLimitTriggered || rightLimitTriggered) {
            handleLimitFlags();  // Debounce and process
            leftLimitTriggered = false;
            rightLimitTriggered = false;
        }
        
        // Process motion commands from queue (every 1ms)
        static uint8_t cmdCounter = 0;
        if (++cmdCounter >= 5) {
            cmdCounter = 0;
            MotionCommand cmd;
            if (xQueueReceive(g_motionCommandQueue, &cmd, 0) == pdTRUE) {
                processMotionCommand(cmd);
            }
        }
        
        // Update homing if in progress
        if (homingInProgress) {
            updateHomingSequence();
        }
        
        // Update status (thread-safe)
        updateMotionStatus();
        
        // Precise 200μs timing
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Minimal ISRs to avoid interfering with FastAccelStepper
volatile bool leftLimitTriggered = false;
volatile bool rightLimitTriggered = false;

void IRAM_ATTR leftLimitISR() {
    leftLimitTriggered = true;  // Single instruction only!
}

void IRAM_ATTR rightLimitISR() {
    rightLimitTriggered = true;  // Single instruction only!
}

// Initialize on Core 0 with interrupts
xTaskCreatePinnedToCore(
    stepperControllerTask,
    "StepperCtrl",
    4096,           // Stack size
    NULL,           // Parameters
    2,              // Priority (higher than normal)
    &stepperTaskHandle,
    0               // Core 0 - CRITICAL!
);

// Set up minimal interrupts
attachInterrupt(digitalPinToInterrupt(LEFT_LIMIT_PIN), leftLimitISR, FALLING);
attachInterrupt(digitalPinToInterrupt(RIGHT_LIMIT_PIN), rightLimitISR, FALLING);
```

## Complete Command Reference

### Motion Commands:
- `MOVE <position>` - Move to absolute position
- `MOVEHOME` - Move to configured home position (percentage of range)
- `HOME` - Run auto-range homing sequence
- `STOP` - Stop with deceleration
- `ESTOP` - Emergency stop (immediate)
- `ENABLE` - Enable motor (default on startup)
- `DISABLE` - Disable motor (allows manual movement)
- `TEST` - Run automated range test (requires homing first)
- `TEST2` or `RANDOMTEST` - Run random position test (10 positions)

### Information Commands:
- `STATUS` - Show current system status
- `CONFIG` - Display complete configuration with metadata
- `PARAMS` - List all configurable parameters with ranges
- `HELP` - Show command help

### Configuration Commands:
- `CONFIG SET <param> <value>` - Set parameter value
- `CONFIG RESET <param>` - Reset single parameter to default
- `CONFIG RESET motion` - Reset all motion parameters
- `CONFIG RESET dmx` - Reset all DMX parameters  
- `CONFIG RESET` - Factory reset all settings

### Interface Commands:
- `ECHO ON/OFF` - Control command echo
- `VERBOSE 0-3` - Set output verbosity
- `JSON ON/OFF` - Switch to JSON output mode
- `STREAM ON/OFF` - Enable/disable status streaming

### JSON API Commands:
**Important**: JSON commands should be sent when in normal mode (`JSON OFF`). The `JSON ON/OFF` command only affects output format, not input parsing.

#### Examples:
```json
{"command":"status"}
{"command":"config"}
{"command":"move","position":1000}
{"command":"home"}
{"command":"stop"}
{"command":"enable"}
{"command":"disable"}
{"command":"config","set":{"maxSpeed":2000}}
{"command":"config","set":{"homingSpeed":1500}}
{"command":"config","set":{"dmxStartChannel":10,"dmxScale":5.0}}
```

### Configurable Parameters:
- **Motion**: `maxSpeed`, `acceleration`, `homingSpeed`
- **DMX**: `dmxStartChannel`, `dmxScale`, `dmxOffset`
- **System**: `verbosity`

Use `PARAMS` command for full parameter details with ranges and defaults.

## Current Status Summary (v4.1.13 - 2025-02-08)

### 🏆 Production-Ready System

**Completed Modules:**
✅ **Phase 1**: Hardware foundation and module framework - COMPLETE  
✅ **Phase 2**: Configuration management with flash storage - COMPLETE  
✅ **Phase 3**: Interactive command interface (human & JSON) - COMPLETE  
✅ **Phase 4**: Motion control with ODStepper integration - COMPLETE  
✅ **Phase 5**: WebInterface module - COMPLETE (Core system component)

**Active Development:**
🚧 **Phase 6**: DMXReceiver module - IN PROGRESS (Phase 1 of 8 complete)

**Future Enhancements:**
🔄 **Phase 7**: SafetyMonitor module - OPTIONAL (safety already integrated)
🔄 **Phase 8**: Advanced features - Data logging, OTA updates, etc.

### 🌟 Key Achievements

1. **Industrial-Grade Motion Control**
   - Hardware timer-based step generation
   - Smooth trapezoidal profiles up to 10kHz
   - Auto-range homing adapts to any installation
   - Professional safety with fault latching

2. **Dual Control Interfaces**
   - **Serial**: Complete command system with JSON API
   - **Web**: Real-time responsive interface at 192.168.4.1
   - Both interfaces fully integrated and thread-safe

3. **Enterprise Features**
   - Configuration persistence across reboots
   - Thread-safe dual-core architecture
   - Comprehensive error handling and recovery
   - Built-in test and diagnostic functions

4. **Safety First Design**
   - Hardware limit switches with proper debouncing
   - Emergency stop with fault latching
   - Position limits enforcement
   - Homing required before movement

### 🚀 Latest Enhancements (2025-02-07)

- **Fixed DMX Zero Value Behavior**: DMX speed/acceleration channels now properly scale from minimum to maximum (0=slow, 255=fast) instead of jumping to defaults
- **Improved Serial Output**: Removed "Task alive" messages that were corrupting output
- **Better DMX Debugging**: Added DMX_DEBUG_ENABLED flag for cleaner operation when not debugging
- **Channel Monitoring**: Added detection for stuck LSB and channel wake events
- **Fixed DMX Idle Timeout Bug**: DMX control no longer becomes unresponsive after periods of inactivity. Added 30-second position tracking timeout to ensure updates resume properly (v4.1.8)
- **Fixed Homing Speed Override**: Homing sequence now consistently uses the configured `homingSpeed` parameter throughout the entire sequence, including the final move to home position (v4.1.7)
- **DMX Development Progress**: Phase 1-4 complete with position, speed, and acceleration control
- **Improved DMX Channel Layout**: Position MSB/LSB adjacent (channels 0-1)
- **Core 0 DMX Task**: Real-time processing without affecting motion control

- **WebInterface as Core Component**: Full web control with real-time updates
- **Test Functions in Web UI**: TEST and TEST2 buttons for validation
- **Enhanced Safety**: Limit faults now trigger proper EMERGENCY_STOP state
- **System Information Display**: Added real-time system info panel showing version, uptime, memory usage, and task statistics
- **Complete Documentation**: All features documented with examples
- **Fixed Web Interface Stress Test**: TEST button now runs continuous stress test between 10% and 90% of range until stopped (matching serial interface behavior)
- **Renamed TEST2**: Web interface TEST2 button renamed to "RANDOM MOVES" for better clarity
- **Enhanced Limits Tab**: Position limits configuration now requires homing first, displays detected physical range, and validates entries against hardware limits
- **Configurable Home Position**: Home position is now configurable as a percentage of detected range (0-100%), ensuring it's always within bounds and adapts to mechanical installation
- **Move to Home Button**: Added convenient "MOVE TO HOME" button in web interface that moves to the configured home position with a single click
- **Auto-Home Options**: Added checkboxes for "Home on Boot" and "Home on E-Stop" for automatic initialization and recovery

### 🔧 Bug Fixes (v4.1.1)

- **Fixed Auto-Home on E-Stop**: 
  - Now properly triggers from both IDLE and COMPLETE homing states
  - Automatically clears limit fault before attempting to home
  - Uses correct processMotionCommand() instead of direct function calls
  - Added debug output to track auto-home state during 2-second delay

### 🔧 Bug Fixes (v4.1.10)

- **Fixed DMX Channel Cache Corruption**: 
  - DMX values no longer randomly drop to zero during operation
  - Channel cache only updates when receiving valid new DMX packets
  - Added detection and warnings for sudden zero value transitions

- **Improved DMX Zero Value Handling**:
  - Speed/acceleration channels at 0 now use system defaults
  - Prevents slow motion (10 steps/sec) when DMX console sends zero values
  - Maintains smooth operation even with problematic DMX sources

- **Enhanced Debug Output**:
  - Shows all 5 DMX channels for complete visibility
  - Detects stuck LSB in 16-bit position mode
  - Tracks channel wake-up from zero state
  - Cleaner message flow with integrated task status

The SkullStepperV4 system is now a complete, production-ready stepper control solution suitable for professional installations requiring reliable, safe, and user-friendly operation.