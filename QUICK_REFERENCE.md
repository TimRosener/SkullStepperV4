# SkullStepperV4 Quick Reference

## Essential Commands

### First Time Setup
1. `HOME` - **REQUIRED FIRST** - Detects range
2. `CONFIG SET homePositionPercent 50` - Set home position (0-100%)
3. `MOVEHOME` - Test home position

### Basic Operation
- `MOVE <position>` - Move to position in steps
- `MOVEHOME` - Return to home position
- `STOP` - Controlled stop
- `ESTOP` - Emergency stop
- `STATUS` - Current position and state

### Configuration
- `CONFIG` - View all settings
- `CONFIG SET <param> <value>` - Change setting
- `PARAMS` - List parameters with ranges

### Testing
- `TEST` - Continuous range test (10-90%)
- `TEST2` - Random position test (10 moves)

## Web Interface

### Connect
1. WiFi: "SkullStepper" (no password)
2. Browser: http://192.168.4.1

### Controls
- **HOME** - Run homing sequence
- **Position Slider** - Direct position control
- **Move to Home** - One-click home return
- **STOP** - Stop motion
- **TEST/TEST2** - Run tests

## Safety

### Limit Switches
- Motion stops immediately on limit
- Requires HOME to clear fault
- Both limits must work properly

### Emergency Stop
- Serial: Type `ESTOP`
- Web: Click red STOP button
- Hardware: Trigger any limit

## Common Issues

### "Not homed" Error
- Run `HOME` command first
- Wait for completion
- Try movement again

### Limit Fault
- Motion blocked after limit hit
- Run `HOME` to clear
- Check for obstructions

### Web Not Loading
- Reconnect to "SkullStepper" WiFi
- Try different browser
- Check http://192.168.4.1

## Parameters

| Parameter | Range | Default | Unit |
|-----------|-------|---------|------|
| maxSpeed | 0-10000 | 1000 | steps/sec |
| acceleration | 0-20000 | 500 | steps/sec² |
| homingSpeed | 0-10000 | 940 | steps/sec |
| homePositionPercent | 0-100 | 50 | % |

## Status Meanings

- **NOT_INITIALIZED** - System starting
- **READY** - Ready for commands
- **HOMING** - Finding limits
- **MOVING** - In motion
- **IDLE** - At position
- **ERROR** - Fault condition
- **EMERGENCY_STOP** - E-stop active

---
*Version 4.1.0 - Production Ready*
