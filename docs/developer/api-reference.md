# API Reference - SkullStepperV4

## Overview

SkullStepperV4 provides three primary interfaces for control and monitoring:
- **Serial Interface**: Human-readable and JSON commands via USB/UART
- **Web Interface**: HTTP REST API and WebSocket for real-time updates
- **DMX Interface**: DMX512 protocol for theatrical control

## Serial Interface API

### Connection Parameters
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

### Command Format

#### Human-Readable Commands
```
COMMAND [param1] [param2] ...
```
Commands are case-insensitive. Parameters are space-separated.

#### JSON Commands
```json
{"cmd":"COMMAND","param1":value1,"param2":value2}
```

### Motion Commands

#### HOME
Initiates automatic homing sequence to detect range.
```
HOME
{"cmd":"HOME"}
```
**Response**: Status updates during homing, final range report

#### MOVE
Move to absolute position in steps.
```
MOVE <position>
{"cmd":"MOVE","pos":12345}
```
**Parameters**:
- `position`: Target position in steps (0 to maxPosition)

**Response**: Acknowledgment or error

#### MOVEHOME
Move to configured home position.
```
MOVEHOME
{"cmd":"MOVEHOME"}
```
**Response**: Acknowledgment

#### STOP
Controlled deceleration stop.
```
STOP
{"cmd":"STOP"}
```
**Response**: Acknowledgment

#### ESTOP
Emergency stop (immediate, no deceleration).
```
ESTOP
{"cmd":"ESTOP"}
```
**Response**: System enters ERROR state

### Configuration Commands

#### CONFIG
Display all configuration parameters.
```
CONFIG
{"cmd":"CONFIG"}
```
**Response**: All parameters with current values

#### CONFIG SET
Modify configuration parameter.
```
CONFIG SET <parameter> <value>
{"cmd":"CONFIG","action":"SET","param":"maxSpeed","value":20000}
```

**Available Parameters**:
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| maxSpeed | int | 100-50000 | 20000 | Maximum speed (steps/s) |
| acceleration | int | 1000-500000 | 50000 | Acceleration (steps/s²) |
| homingSpeed | int | 100-50000 | 5000 | Homing speed (steps/s) |
| homingAcceleration | int | 1000-500000 | 10000 | Homing acceleration |
| homePositionPercent | float | 0-100 | 50.0 | Home position (% of range) |
| limitDebounceMs | int | 1-100 | 10 | Limit switch debounce (ms) |
| dmxEnabled | bool | 0-1 | 0 | Enable DMX control |
| dmxStartChannel | int | 1-508 | 1 | DMX start channel |
| stepsPerUnit | float | 0.1-10000 | 1.0 | Steps per physical unit |
| softLimitEnabled | bool | 0-1 | 1 | Enable software limits |
| softLimitMin | int | 0-2147483647 | 0 | Minimum position |
| softLimitMax | int | 0-2147483647 | 100000 | Maximum position |
| invertDirection | bool | 0-1 | 0 | Invert motor direction |
| verbosity | int | 0-3 | 1 | Debug output level |

#### CONFIG SAVE
Save configuration to flash memory.
```
CONFIG SAVE
{"cmd":"CONFIG","action":"SAVE"}
```
**Response**: Confirmation or error

#### CONFIG RESET
Reset to factory defaults.
```
CONFIG RESET
{"cmd":"CONFIG","action":"RESET"}
```
**Response**: Confirmation, system restarts

### Status Commands

#### STATUS
Get current system status.
```
STATUS
{"cmd":"STATUS"}
```

**Response** (JSON mode):
```json
{
  "position": 12345,
  "targetPosition": 12345,
  "speed": 0,
  "state": "IDLE",
  "homed": true,
  "limitLeft": false,
  "limitRight": false,
  "motorEnabled": true,
  "fault": 0,
  "maxPosition": 50000,
  "homePosition": 25000,
  "dmxEnabled": true,
  "dmxChannels": [0,0,0,0,0]
}
```

#### PARAMS
List all parameters with descriptions and ranges.
```
PARAMS
{"cmd":"PARAMS"}
```

### Test Commands

#### TEST
Run continuous range test (10-90% of range).
```
TEST
{"cmd":"TEST"}
```
**Response**: Continuous status updates

#### TEST2
Run random position test (10 random moves).
```
TEST2
{"cmd":"TEST2"}
```
**Response**: Status updates for each move

### System Commands

#### HELP
Display command help.
```
HELP
{"cmd":"HELP"}
```

#### RESTART
Restart the system.
```
RESTART
{"cmd":"RESTART"}
```

#### VERSION
Get firmware version.
```
VERSION
{"cmd":"VERSION"}
```
**Response**: "SkullStepperV4 v4.1.13"

## Web Interface API

### Base URL
```
http://192.168.4.1
```

### REST API Endpoints

#### GET /
Main web interface HTML page.

#### GET /status
Get current system status as JSON.

**Response**:
```json
{
  "position": 12345,
  "targetPosition": 12345,
  "speed": 0,
  "state": "IDLE",
  "homed": true,
  "limitLeft": false,
  "limitRight": false,
  "motorEnabled": true,
  "fault": 0,
  "maxPosition": 50000,
  "homePosition": 25000,
  "freeHeap": 150000,
  "taskStatus": {
    "motion": "running",
    "serial": "running",
    "web": "running",
    "dmx": "running"
  }
}
```

#### POST /command
Send command to system.

**Request Body**:
```json
{
  "command": "MOVE",
  "value": 12345
}
```

**Available Commands**:
- `HOME`: Start homing
- `MOVE`: Move to position (requires `value`)
- `STOP`: Stop motion
- `ESTOP`: Emergency stop
- `MOVEHOME`: Move to home position
- `TEST`: Start range test
- `TEST2`: Start random test

**Response**: 
- 200 OK: Command accepted
- 400 Bad Request: Invalid command
- 503 Service Unavailable: System busy

#### GET /config
Get all configuration parameters.

**Response**:
```json
{
  "maxSpeed": 20000,
  "acceleration": 50000,
  "homingSpeed": 5000,
  "homingAcceleration": 10000,
  "homePositionPercent": 50.0,
  "limitDebounceMs": 10,
  "dmxEnabled": false,
  "dmxStartChannel": 1,
  "stepsPerUnit": 1.0,
  "softLimitEnabled": true,
  "softLimitMin": 0,
  "softLimitMax": 100000,
  "invertDirection": false,
  "verbosity": 1
}
```

#### POST /config
Update configuration parameters.

**Request Body**:
```json
{
  "maxSpeed": 25000,
  "acceleration": 75000
}
```

**Response**: 
- 200 OK: Configuration updated
- 400 Bad Request: Invalid parameters

### WebSocket API

#### Connection
```javascript
ws://192.168.4.1:81
```

#### Status Updates
Automatic status broadcasts every 100ms:
```json
{
  "type": "status",
  "data": {
    "position": 12345,
    "targetPosition": 12345,
    "speed": 1000,
    "state": "MOVING",
    "homed": true,
    "limitLeft": false,
    "limitRight": false,
    "motorEnabled": true,
    "fault": 0
  }
}
```

#### DMX Updates
When DMX is enabled:
```json
{
  "type": "dmx",
  "data": {
    "channels": [128, 64, 100, 50, 1],
    "position": 32896,
    "speed": 19607,
    "acceleration": 25000,
    "mode": "CONTROL"
  }
}
```

#### Error Messages
```json
{
  "type": "error",
  "message": "Limit switch triggered",
  "code": 1
}
```

## DMX Interface API

### DMX Configuration
- **Protocol**: DMX512
- **Channels Used**: 5 consecutive channels
- **Update Rate**: 44Hz maximum
- **Start Channel**: Configurable (1-508)

### Channel Mapping

| Channel | Function | Range | Description |
|---------|----------|-------|-------------|
| Start+0 | Position High | 0-255 | Upper 8 bits of position |
| Start+1 | Position Low | 0-255 | Lower 8 bits of position |
| Start+2 | Speed | 0-255 | Speed scaling (% of max) |
| Start+3 | Acceleration | 0-255 | Acceleration scaling (% of max) |
| Start+4 | Mode | 0-255 | Control mode |

### Mode Values
- **0**: STOP - Stop all motion
- **1-254**: CONTROL - Normal position control
- **255**: HOME - Initiate homing

### Position Calculation
```
Position = (HighByte × 256 + LowByte) × (MaxPosition / 65535)
```

### Speed Calculation
```
Speed = (SpeedByte / 255) × MaxSpeed
```

### Acceleration Calculation
```
Acceleration = (AccelByte / 255) × MaxAcceleration
```

## Error Codes

### Fault Flags (Bitfield)

| Bit | Value | Name | Description |
|-----|-------|------|-------------|
| 0 | 0x01 | FAULT_LIMIT_LEFT | Left limit triggered |
| 1 | 0x02 | FAULT_LIMIT_RIGHT | Right limit triggered |
| 2 | 0x04 | FAULT_DRIVER_ALARM | Driver alarm active |
| 3 | 0x08 | FAULT_POSITION_ERROR | Position error detected |
| 4 | 0x10 | FAULT_NOT_HOMED | Motion before homing |
| 5 | 0x20 | FAULT_COMMUNICATION | Communication timeout |
| 6 | 0x40 | FAULT_SYSTEM | System error |
| 7 | 0x80 | FAULT_EMERGENCY_STOP | E-stop activated |

### System States

| State | Description |
|-------|-------------|
| IDLE | Ready for commands |
| MOVING | Motion in progress |
| HOMING | Homing sequence active |
| ERROR | Fault condition (requires HOME to clear) |
| NOT_HOMED | Waiting for initial homing |

## Response Formats

### Success Response (Serial)
```
OK
OK: Command completed successfully
```

### Error Response (Serial)
```
ERROR: Description of error
ERROR: Invalid parameter: paramName
ERROR: System not homed
```

### JSON Success Response
```json
{
  "status": "success",
  "message": "Command executed"
}
```

### JSON Error Response
```json
{
  "status": "error",
  "message": "Description of error",
  "code": 1
}
```

## Rate Limits

### Command Processing
- Serial: 100 commands/second maximum
- Web API: 50 requests/second maximum
- WebSocket: 100 messages/second maximum

### Status Updates
- Serial STATUS: On demand
- Web API /status: 10Hz polling recommended
- WebSocket: 10Hz automatic broadcast

### Configuration Changes
- Maximum 10 changes per second
- Flash writes limited to 1 per second

## Best Practices

### Command Sequencing
1. Always HOME before first motion
2. Wait for IDLE state before new commands
3. Use STOP for controlled stops
4. Use ESTOP only for emergencies

### Error Handling
1. Check system state before commands
2. Monitor fault flags
3. Clear faults with HOME command
4. Implement timeout handling

### Performance Optimization
1. Use WebSocket for real-time monitoring
2. Batch configuration changes
3. Minimize flash writes
4. Use appropriate verbosity level

### Safety
1. Implement client-side limit checking
2. Add confirmation for critical commands
3. Monitor communication timeouts
4. Have emergency stop accessible

## Examples

### Python Serial Control
```python
import serial
import json

# Connect
ser = serial.Serial('/dev/ttyUSB0', 115200)

# Send command
ser.write(b'HOME\n')

# JSON command
cmd = {"cmd": "MOVE", "pos": 10000}
ser.write(json.dumps(cmd).encode() + b'\n')

# Read response
response = ser.readline().decode()
```

### JavaScript WebSocket
```javascript
const ws = new WebSocket('ws://192.168.4.1:81');

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Position:', data.data.position);
};

// Send command via HTTP
fetch('http://192.168.4.1/command', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({command: 'MOVE', value: 10000})
});
```

### DMX Control (Pseudo-code)
```
// Set DMX channels for position 32768 at 50% speed
DMX[1] = 128  // Position high byte
DMX[2] = 0    // Position low byte
DMX[3] = 128  // 50% speed
DMX[4] = 128  // 50% acceleration
DMX[5] = 1    // CONTROL mode
```