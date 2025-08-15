# SkullStepperV4 Serial Interface Manual

## Table of Contents
1. [Getting Started](#getting-started)
2. [Connection Setup](#connection-setup)
3. [Interface Modes](#interface-modes)
4. [Human-Readable Commands](#human-readable-commands)
5. [JSON API Reference](#json-api-reference)
6. [Configuration Parameters](#configuration-parameters)
7. [Error Handling](#error-handling)
8. [Examples and Tutorials](#examples-and-tutorials)
9. [Troubleshooting](#troubleshooting)

---

## Getting Started

The SkullStepperV4 Serial Interface provides complete interactive control over your stepper motor system through both human-readable commands and a structured JSON API.

### Quick Start
1. Connect to serial port at **115200 baud**
2. You'll see the prompt: `skull> `
3. Type `HELP` for complete command reference
4. Type `STATUS` to see current system state

---

## Connection Setup

### Serial Configuration
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Line Ending**: Newline (LF) or Carriage Return + Line Feed (CRLF)

### Using Arduino Serial Monitor
1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. Set line ending to **"Newline"** or **"Both NL & CR"**
4. You should see: `skull> ` prompt

### Using Terminal Applications
```bash
# Linux/Mac
screen /dev/ttyUSB0 115200
# or
minicom -D /dev/ttyUSB0 -b 115200

# Windows
putty -serial COM3 -sercfg 115200,8,n,1
```

---

## Interface Modes

The system supports two output modes that can be switched dynamically:

### Human Mode (Default)
- **Readable text responses**
- **Interactive prompts**
- **Formatted status displays**
- **Color-coded messages** (OK, ERROR, INFO)

```
skull> STATUS
=== System Status ===
System State: READY
Position: 0 steps (target: 0)
Speed: 0.0 steps/sec
Stepper: DISABLED
...
```

### JSON Mode
- **Structured JSON responses**
- **Machine-readable format**
- **Consistent response format**
- **Perfect for automation**

```
skull> JSON ON
{"status":"ok"}
skull> STATUS
{"systemState":2,"position":{"current":0,"target":0},"speed":0.0,...}
```

**Switch Modes:**
```
skull> JSON ON     # Enable JSON mode
skull> JSON OFF    # Return to human mode
skull> JSON        # Show current mode
```

---

## Human-Readable Commands

All commands are case-insensitive and can be typed at the `skull> ` prompt.

### Motion Commands

#### MOVE
Move to an absolute position.
```
MOVE <position>
```
**Examples:**
```
skull> MOVE 1000      # Move to position 1000 steps
skull> MOVE -500      # Move to position -500 steps
skull> MOVE 0         # Move to home position
```

#### HOME
Start the homing sequence.
```
HOME
```
**Example:**
```
skull> HOME
INFO: Motion command queued
OK
```

#### STOP
Stop current motion with normal deceleration.
```
STOP
```

#### ESTOP / EMERGENCY
Emergency stop with maximum deceleration.
```
ESTOP
EMERGENCY
```

#### ENABLE / DISABLE
Control stepper motor power.
```
ENABLE     # Enable stepper motor
DISABLE    # Disable stepper motor
```

### Information Commands

#### STATUS
Display comprehensive system status.
```
STATUS
```
**Output:**
```
=== System Status ===
System State: READY
Position: 1000 steps (target: 1000)
Speed: 0.0 steps/sec
Stepper: ENABLED
Max Speed: 2000.0 steps/sec
Acceleration: 1500.0 steps/sec²
DMX Channel: 10
DMX Scale: 5.00 steps/unit
DMX Offset: 128 steps
Uptime: 45230 ms
=====================
```

#### CONFIG
Display complete configuration with metadata.
```
CONFIG
```
**Output:** Comprehensive JSON with parameter ranges, units, and constraints.

#### HELP
Show complete command reference.
```
HELP
```

### Configuration Commands

#### CONFIG SET
Set individual configuration parameters.
```
CONFIG SET <parameter> <value>
```

**Motion Parameters:**
```
CONFIG SET maxSpeed 2000        # Set maximum speed
CONFIG SET acceleration 1500    # Set acceleration rate
CONFIG SET deceleration 1200    # Set deceleration rate
CONFIG SET jerk 2000           # Set jerk limitation
```

**DMX Parameters:**
```
CONFIG SET dmxStartChannel 10   # Set DMX channel (1-512)
CONFIG SET dmxScale 5.0        # Set scaling factor
CONFIG SET dmxOffset 128       # Set position offset
```

**System Parameters:**
```
CONFIG SET verbosity 3         # Set debug level (0-3)
```

#### CONFIG RESET
Reset parameters to default values.

**Individual Parameters:**
```
CONFIG RESET maxSpeed          # Reset max speed to 1000
CONFIG RESET acceleration      # Reset to 500 steps/sec²
CONFIG RESET dmxStartChannel   # Reset to channel 1
CONFIG RESET dmxScale          # Reset to 1.0
CONFIG RESET dmxOffset         # Reset to 0
CONFIG RESET verbosity         # Reset to level 2
```

**Parameter Groups:**
```
CONFIG RESET motion            # Reset all motion parameters
CONFIG RESET dmx              # Reset all DMX parameters
```

**Factory Reset:**
```
CONFIG RESET                  # Reset everything to defaults
```

### Interface Control Commands

#### ECHO
Control command echo.
```
ECHO ON      # Enable command echo
ECHO OFF     # Disable command echo
ECHO         # Show current setting
```

#### VERBOSE
Set output verbosity level.
```
VERBOSE <level>    # Set level (0-3)
VERBOSE           # Show current level
```

**Verbosity Levels:**
- **0**: Minimal (errors only)
- **1**: Normal (+ info messages)  
- **2**: Verbose (+ status updates)
- **3**: Debug (+ debug messages)

#### STREAM
Control automatic status streaming.
```
STREAM ON     # Enable periodic status updates
STREAM OFF    # Disable automatic updates  
STREAM        # Show current setting
```

---

## JSON API Reference

Send JSON commands as single-line strings. All responses are in JSON format when JSON mode is enabled.

### Command Structure
```json
{
  "command": "<command_name>",
  "parameter": "value",
  ...
}
```

### Motion Commands

#### Move to Position
```json
{"command":"move","position":1000}
```
**Response:**
```json
{"status":"ok","message":"Move command queued"}
```

#### Home
```json
{"command":"home"}
```

#### Stop
```json
{"command":"stop"}
```

#### Emergency Stop
```json
{"command":"estop"}
```

#### Enable/Disable
```json
{"command":"enable"}
{"command":"disable"}
```

### Information Commands

#### Get Status
```json
{"command":"status"}
```
**Response:**
```json
{
  "systemState": 2,
  "position": {"current": 1000, "target": 1000},
  "speed": 0.0,
  "stepperEnabled": true,
  "uptime": 45230,
  "config": {
    "maxSpeed": 2000.0,
    "acceleration": 1500.0,
    "dmxChannel": 10,
    "dmxScale": 5.0,
    "dmxOffset": 128
  }
}
```

#### Get Configuration
```json
{"command":"config","get":"all"}
```
**Response:** Complete configuration with metadata, ranges, and units.

### Configuration Commands

#### Set Single Parameter
```json
{"command":"config","set":{"maxSpeed":2000}}
```

#### Set Multiple Parameters
```json
{
  "command":"config",
  "set":{
    "maxSpeed":2500,
    "acceleration":1800,
    "dmxStartChannel":15
  }
}
```

#### Set DMX Configuration
```json
{
  "command":"config",
  "set":{
    "dmxScale":10.0,
    "dmxOffset":500,
    "dmxStartChannel":10
  }
}
```

### Response Format

**Success Response:**
```json
{"status":"ok","message":"Configuration updated"}
```

**Error Response:**
```json
{"status":"error","message":"Invalid parameter value"}
```

**Info Response:**
```json
{"status":"info","message":"DMX offset updated successfully"}
```

---

## Configuration Parameters

### Motion Profile Parameters

| Parameter | Range | Units | Default | Description |
|-----------|-------|-------|---------|-------------|
| `maxSpeed` | 0-10000 | steps/sec | 1000 | Maximum velocity |
| `acceleration` | 0-20000 | steps/sec² | 500 | Acceleration rate |
| `deceleration` | 0-20000 | steps/sec² | 500 | Deceleration rate |
| `jerk` | 0-50000 | steps/sec³ | 1000 | Jerk limitation |

### DMX Configuration Parameters

| Parameter | Range | Units | Default | Description |
|-----------|-------|-------|---------|-------------|
| `dmxStartChannel` | 1-512 | - | 1 | DMX channel to monitor |
| `dmxScale` | ≠0 | steps/unit | 1.0 | Position scaling factor |
| `dmxOffset` | any | steps | 0 | Position offset |

### DMX Position Calculation
```
Final Position = (DMX_Value × dmxScale) + dmxOffset
```

**Examples:**
- DMX=128, Scale=10.0, Offset=500 → Position=1780
- DMX=255, Scale=5.0, Offset=0 → Position=1275
- DMX=0, Scale=10.0, Offset=1000 → Position=1000

### System Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `verbosity` | 0-3 | 2 | Output verbosity level |
| `statusUpdateInterval` | 10-10000 ms | 100 | Status update frequency |
| `enableSerialOutput` | true/false | true | Enable status output |

---

## Error Handling

### Error Types

#### Validation Errors
```
ERROR: Invalid speed value (must be 0-10000 steps/sec)
ERROR: DMX start channel must be 1-512
ERROR: DMX scale cannot be zero
```

#### System Errors
```
ERROR: Configuration not available
ERROR: Failed to save configuration to flash
ERROR: Motion command queue full
```

#### Command Errors
```
ERROR: Unknown command. Type HELP for available commands
ERROR: CONFIG SET requires parameter and value
ERROR: Unknown parameter
```

### JSON Error Responses
```json
{"status":"error","message":"Invalid parameter value"}
{"status":"error","message":"JSON parse error: Invalid character"}
{"status":"error","message":"Missing command field"}
```

### Debugging Errors

1. **Enable Debug Output:**
   ```
   skull> VERBOSE 3
   ```

2. **Check Configuration:**
   ```
   skull> CONFIG
   ```

3. **Verify System Status:**
   ```
   skull> STATUS
   ```

---

## Examples and Tutorials

### Basic Motion Control

#### Example 1: Simple Movement
```
skull> ENABLE           # Enable stepper motor
OK
skull> MOVE 1000        # Move to position 1000
INFO: Motion command queued
OK
skull> STATUS           # Check position
Position: 1000 steps (target: 1000)
```

#### Example 2: Homing Sequence
```
skull> HOME             # Start homing
INFO: Motion command queued
OK
skull> STATUS           # Check status
Position: 0 steps (target: 0)
```

### Configuration Management

#### Example 3: Speed Adjustment
```
skull> CONFIG SET maxSpeed 2000    # Increase max speed
INFO: Max speed updated successfully
OK
skull> CONFIG SET acceleration 1500 # Increase acceleration
INFO: Acceleration updated successfully
OK
```

#### Example 4: DMX Setup
```
skull> CONFIG SET dmxStartChannel 10  # Set DMX channel
OK
skull> CONFIG SET dmxScale 5.0       # Set scaling
OK
skull> CONFIG SET dmxOffset 500      # Set offset
OK
```

### JSON API Usage

#### Example 5: JSON Configuration
```json
{"command":"config","set":{"maxSpeed":2500,"acceleration":1800}}
```
**Response:**
```json
{"status":"ok","message":"Configuration updated"}
```

#### Example 6: JSON Motion Control
```json
{"command":"move","position":1500}
```
```json
{"command":"status"}
```

### Advanced Features

#### Example 7: Factory Reset
```
skull> CONFIG RESET     # Reset everything
INFO: Performing factory reset...
INFO: Factory reset completed - all settings restored to defaults
OK
```

#### Example 8: Parameter Group Reset
```
skull> CONFIG RESET motion    # Reset only motion parameters
INFO: All motion settings reset to defaults
OK
```

### Interface Customization

#### Example 9: Clean Output Mode
```
skull> ECHO OFF         # Disable command echo
OK
skull> STREAM OFF       # Disable auto status
OK
skull> VERBOSE 1        # Minimal verbosity
OK
```

#### Example 10: Debug Mode
```
skull> VERBOSE 3        # Full debug output
OK
skull> STREAM ON        # Enable status streaming
OK
```

---

## Troubleshooting

### Common Issues

#### No Response from Serial
**Problem:** No `skull> ` prompt appears
**Solutions:**
1. Check baud rate (must be 115200)
2. Verify correct serial port
3. Check line ending settings
4. Try pressing Enter to get prompt

#### Commands Not Working
**Problem:** Commands return "Unknown command"
**Solutions:**
1. Check spelling (commands are case-insensitive)
2. Ensure proper syntax (spaces between parameters)
3. Type `HELP` for command reference

#### Configuration Not Saving
**Problem:** Settings revert after restart
**Solutions:**
1. Check for "Failed to save to flash" errors
2. Enable debug: `VERBOSE 3`
3. Try factory reset: `CONFIG RESET`

#### JSON Parse Errors
**Problem:** JSON commands fail
**Solutions:**
1. Validate JSON syntax (use online validator)
2. Send as single line (no line breaks)
3. Use double quotes for strings
4. Check for trailing commas

### Debug Procedures

#### Full System Check
```
skull> VERBOSE 3        # Enable full debug
skull> STATUS           # Check system state
skull> CONFIG           # Verify configuration
skull> HELP             # Review available commands
```

#### Configuration Verification
```
skull> CONFIG SET maxSpeed 1500    # Test parameter setting
skull> CONFIG                      # Verify change saved
skull> CONFIG RESET maxSpeed       # Test reset function
skull> CONFIG                      # Verify reset worked
```

#### Motion Test Sequence
```
skull> ENABLE           # Enable motor
skull> MOVE 100         # Small test move
skull> STATUS           # Check position
skull> HOME             # Return to home
skull> DISABLE          # Disable motor
```

### Getting Help

1. **Interactive Help:**
   ```
   skull> HELP
   ```

2. **Parameter Information:**
   ```
   skull> CONFIG
   ```

3. **System Status:**
   ```
   skull> STATUS
   ```

4. **Debug Output:**
   ```
   skull> VERBOSE 3
   ```

---

## Command Reference Quick Card

### Motion
- `MOVE <pos>` - Move to position
- `HOME` - Start homing
- `STOP` - Stop motion
- `ESTOP` - Emergency stop
- `ENABLE`/`DISABLE` - Motor control

### Configuration  
- `CONFIG` - Show config
- `CONFIG SET <param> <value>` - Set parameter
- `CONFIG RESET <param>` - Reset parameter
- `CONFIG RESET` - Factory reset

### Information
- `STATUS` - System status
- `HELP` - Command help

### Interface
- `ECHO ON/OFF` - Command echo
- `VERBOSE 0-3` - Debug level
- `JSON ON/OFF` - Output mode
- `STREAM ON/OFF` - Auto status

### JSON Commands
```json
{"command":"move","position":1000}
{"command":"config","set":{"maxSpeed":2000}}
{"command":"status"}
```

---

*SkullStepperV4 Serial Interface Manual v4.1.13*