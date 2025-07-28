# SkullStepperV4 - ESP32-S3 Closed-Loop Stepper Control System

## Project Overview

This is a comprehensive modular stepper motor control system with DMX input and extensive configurability, designed for the ESP32-S3 microcontroller with **CL57Y closed-loop stepper driver**.

## Development Status

**Current Phase: Phase 4 (StepperController with S-Curve) - IN PROGRESS 🔄**

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

### 🔄 Phase 4: StepperController Module (IN PROGRESS - ODStepper Implementation)
- [x] **Thread-Safe Infrastructure** - GlobalInfrastructure.cpp with FreeRTOS mutexes/queues
- [ ] **ODStepper Integration** - Using ODStepper library (FastAccelStepper with open-drain)
- [ ] **Motion Profile Management** - Thread-safe wrappers for speed/acceleration control
- [ ] **Dynamic Target Updates** - Seamless position changes while moving (for DMX)
- [ ] **Software Position Limits** - Min/max position enforcement
- [ ] **Hardware Limit Switches** - LEFT_LIMIT and RIGHT_LIMIT implementation
- [ ] **Homing Routine** - Find home position using limit switches
- [ ] **Integration with SerialInterface** - Command queue processing
- [ ] **CL57Y ALARM Monitoring** - Position following error detection
- [ ] **Safety Features** - Limit monitoring, emergency stop, position validation

### 🔄 Next Phases (Planned)
- [ ] **Phase 5:** SafetyMonitor Module (Limit switches, CL57Y alarm monitoring)
- [ ] **Phase 6:** DMXReceiver Module (DMX512 packet reception)

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

### 🔄 StepperController (Core 0 - Real-Time) - PHASE 4 IN PROGRESS (Planning)
**Planned Architecture:**
- **ODStepper Integration**: Thread-safe wrapper around ODStepper library
- **Motion Profiles**: Trapezoidal acceleration via FastAccelStepper
- **Dynamic Updates**: Seamless target changes while moving (for DMX)
- **Position Limits**: Software min/max enforcement with safety checks
- **Limit Switches**: Hardware limit detection and homing routine
- **Thread-Safe Operation**: Dedicated mutex for ODStepper access
- **Command Queue**: Process motion commands from SerialInterface

**Key Features to Implement:**
- **Motion Control**: moveTo(), move(), stop(), emergencyStop()
- **Profile Management**: setMaxSpeed(), setAcceleration()
- **Limit Handling**: Check limits before moves, monitor during motion
- **Homing Routine**: Find home using left limit switch
- **Status Reporting**: Position, speed, motion state
- **Safety Features**: Emergency stop on limits, position validation
- **CL57Y ALARM**: Monitor for position following errors

### DMXReceiver (Core 0 - Real-Time) - PLANNED
- Receive DMX512 packets via MAX485 interface
- Validate packet timing and data integrity
- Scale DMX values to system units
- Monitor signal presence and timeout conditions

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

## Phase 4 Current Implementation

### **S-Curve Motion Control (Completed)**

**1. Sigmoid Acceleration Function**
```cpp
float calculateSCurveSpeed(float normalizedTime, float maxSpeed) {
    float k = 6.0f;  // Smoothness factor
    float sigmoid = 1.0f / (1.0f + exp(-k * (normalizedTime - 0.5f)));
    return maxSpeed * sigmoid;
}
```

**2. Hardware Timer-Based Pulse Generation**
```cpp
static esp_timer_handle_t g_stepPulseTimer = nullptr;
static esp_timer_handle_t g_stepIdleTimer = nullptr;
// 25% duty cycle: 75% LOW (step active), 25% HIGH (idle)
```

**3. Thread-Safe Position Tracking**
```cpp
// Track COMMANDED position (not actual motor position)
static volatile int32_t g_currentPosition = 0;
void stepPulseEndISR(void* arg) {
    if (g_directionForward) {
        g_currentPosition++;
    } else {
        g_currentPosition--;
    }
}
```

### **Integration Status (Completed)**
- ✅ **Complete SerialInterface Integration**: All commands and JSON API preserved
- ✅ **Thread-Safe Infrastructure**: FreeRTOS mutexes and queues operational
- ✅ **ESP32 Flash Storage**: Configuration persistence maintained
- ✅ **S-Curve Algorithm**: Ultra-smooth motion profiles implemented

### **Next Phase 4 Tasks**
- [ ] **CL57Y ALARM Integration**: Monitor ALARM pin for following errors
- [ ] **25% Duty Cycle Validation**: Verify pulse timing meets CL57Y requirements
- [ ] **Real-World Testing**: Validate smooth motion and eliminate stepping sounds
- [ ] **Performance Optimization**: Fine-tune for maximum smoothness

## Current Status Summary

✅ **Phase 1**: Hardware foundation and module framework - COMPLETE  
✅ **Phase 2**: Configuration management with flash storage - COMPLETE  
✅ **Phase 3**: Interactive command interface (human & JSON) - COMPLETE  
🔄 **Phase 4**: Motion control with ODStepper integration - IN PROGRESS (implementation)  

**Current State: System has complete interactive control, thread-safe operation, and persistent configuration. Ready for StepperController implementation using ODStepper library.**

**Recent Development (2025-01-28):**
- Implementing ODStepper library integration for motion control
- Thread-safe wrapper design with Core 0 real-time task
- Auto-range detection homing sequence design
- Trapezoidal motion profiles with dynamic target updates for DMX
- Minimal ISR design for limit switches to avoid timing conflicts

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

## Files Status

### **Required Files for Current System:**
- ✅ `skullstepperV4.ino` - Thread-safe main sketch with complete integration
- ✅ `GlobalInfrastructure.cpp` - Thread-safe infrastructure (mutexes, queues, utilities)
- ✅ `GlobalInterface.h` - Complete interface definitions
- ✅ `HardwareConfig.h` - ESP32-S3 pin assignments and constants
- ✅ `StepperController.cpp` - S-curve implementation (in progress)
- ✅ `StepperController.h` - Motion control interface
- ✅ `SerialInterface.cpp` - Complete command system (existing)
- ✅ `SerialInterface.h` - Command interface (existing)
- ✅ `SystemConfig.cpp` - Flash storage implementation (existing)
- ✅ `SystemConfig.h` - Configuration interface (existing)
- 📝 `README.md` - This design document (updated)
- 📝 `SerialInterface_Manual.md` - Complete command reference (existing)

**System is ready for Phase 4 completion and testing of S-curve motion quality.**