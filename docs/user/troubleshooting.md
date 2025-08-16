# Troubleshooting Guide - SkullStepperV4

## Quick Diagnostics

Before diving into specific issues, run these diagnostic commands:

```bash
# Via Serial (115200 baud)
STATUS      # Check system state
CONFIG      # Verify configuration
PARAMS      # List all parameters
TEST        # Run motion test
```

## Common Issues and Solutions

### 1. System Won't Start

#### Symptoms
- No response to commands
- No WiFi network visible
- No serial output

#### Solutions

**Check Power**
- Verify ESP32 has 5V power (or USB connected)
- Check power LED on ESP32 board
- Measure 3.3V on ESP32 3.3V pin
- Ensure ground connections solid

**Check Serial Connection**
- Correct port selected (usually /dev/ttyUSB0 or COM#)
- Baud rate set to 115200
- Try different USB cable
- Check USB CDC mode enabled in Arduino IDE

**Check Firmware**
- Re-upload firmware with correct board settings:
  ```bash
  arduino-cli compile --upload -p /dev/cu.usbmodem11301 \
    --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .
  ```
- Verify ESP32-S3 board package installed
- Check all required libraries installed

### 2. Motor Won't Move

#### Symptoms
- Commands accepted but no motion
- Status shows IDLE but no movement
- Position doesn't change

#### Solutions

**Not Homed**
```
# Motor requires homing before any motion
HOME
```

**Driver Not Enabled**
- Check ENABLE signal (GPIO 16 should be HIGH)
- Verify in STATUS output: `motorEnabled: true`
- Check driver LED (should be green, not red)

**Driver Alarm Active**
- Check ALARM pin status in diagnostics
- Look for red LED on CL57Y driver
- Common causes:
  - Position error (mechanical binding)
  - Overcurrent (motor specs mismatch)
  - Overtemperature

**Power Issues**
- Verify motor power supply (24-48VDC)
- Check driver power LED
- Measure voltage at driver terminals
- Ensure adequate current capacity

**Wiring Problems**
- Verify step/direction connections:
  - STEP (GPIO 7) → PUL+
  - DIR (GPIO 15) → DIR+
  - Common ground → PUL-/DIR-
- Check motor phase connections
- Test continuity of all wires

### 3. Erratic or Unstable Motion

#### Symptoms
- Motor vibrates but doesn't rotate
- Inconsistent movement
- Position drift
- Missed steps

#### Solutions

**Reduce Speed/Acceleration**
```
CONFIG SET maxSpeed 10000
CONFIG SET acceleration 25000
```

**Check Microstepping**
- Verify driver DIP switches match configuration
- Common setting: 1600 steps/revolution
- Update stepsPerUnit if changed

**EMI/Noise Issues**
- Use shielded cable for motor
- Separate motor and signal wiring
- Add ferrite cores to cables
- Ensure proper grounding

**Mechanical Issues**
- Check for binding in mechanism
- Verify belt tension (if used)
- Lubricate linear guides
- Check coupling alignment

### 4. Limit Switches Not Working

#### Symptoms
- Homing fails
- Motion doesn't stop at limits
- False limit triggers

#### Solutions

**Verify Switch Type**
```
# Must be Normally Closed (NC) switches
# Test with multimeter:
# - Not pressed: 0Ω (continuity)
# - Pressed: Open circuit
```

**Check Wiring**
- LEFT_LIMIT → GPIO 17
- RIGHT_LIMIT → GPIO 18
- Switch NC terminal → GPIO
- Switch C terminal → GND

**Test Switches Manually**
```
# Monitor status while pressing switches
STATUS
# Should show limitLeft/limitRight changing
```

**Debouncing Issues**
- Add 100nF capacitor across switch
- Increase software debounce time if needed
- Check for electrical noise

### 5. Web Interface Issues

#### Symptoms
- Can't connect to web interface
- Page doesn't load
- Controls unresponsive

#### Solutions

**WiFi Connection**
1. Connect to WiFi: "SkullStepper" (no password)
2. Browse to: http://192.168.4.1
3. If redirected, disable mobile data

**Browser Issues**
- Try different browser
- Clear browser cache
- Disable ad blockers
- Use incognito/private mode

**WebSocket Problems**
- Check browser console for errors (F12)
- Verify WebSocket connection established
- Look for firewall blocking
- Try disabling VPN

### 6. DMX Not Working

#### Symptoms
- No response to DMX signals
- Incorrect position control
- DMX status shows no data

#### Solutions

**Check DMX Configuration**
```
CONFIG SET dmxEnabled 1
CONFIG SET dmxStartChannel 1
```

**Verify Wiring**
- DMX+ → RS485 module B+
- DMX- → RS485 module A-
- Ground → Common ground
- 120Ω termination at chain end

**Test DMX Signal**
- Use DMX tester or console
- Verify signal on correct channels
- Check DMX refresh rate (should be ~44Hz)

**Channel Mapping**
- Channel 1: Position High Byte
- Channel 2: Position Low Byte
- Channel 3: Speed
- Channel 4: Acceleration
- Channel 5: Mode (0=STOP, 1-254=CONTROL, 255=HOME)

### 7. Configuration Won't Save

#### Symptoms
- Settings lost on power cycle
- CONFIG SAVE reports error
- Factory defaults keep loading

#### Solutions

**Flash Storage Issue**
```
# Try factory reset first
CONFIG RESET

# Then reconfigure and save
CONFIG SET <param> <value>
CONFIG SAVE
```

**Verify Flash Write**
- Check for flash wear (excessive writes)
- Monitor flash operations in debug mode
- Consider reducing save frequency

### 8. Position Errors

#### Symptoms
- Final position incorrect
- Accumulated drift
- Position jumps

#### Solutions

**Calibration**
```
# Re-run homing to recalibrate
HOME

# Verify range detection
STATUS
```

**Mechanical Slippage**
- Check motor coupling tightness
- Verify belt tension
- Look for mechanical play
- Check set screws

**Electrical Noise**
- Check step signal integrity with oscilloscope
- Verify ground connections
- Add shielding if needed

## Error Codes and Meanings

### System Status Codes

| Code | Meaning | Solution |
|------|---------|----------|
| IDLE | Ready for commands | Normal state |
| MOVING | Motion in progress | Wait for completion |
| HOMING | Homing sequence active | Wait for completion |
| ERROR | Fault condition | Run HOME to clear |
| NOT_HOMED | Homing required | Execute HOME command |

### Fault Flags

| Flag | Description | Recovery |
|------|-------------|----------|
| LIMIT_TRIGGERED | Limit switch activated | HOME command |
| DRIVER_ALARM | Driver reports error | Check driver, then HOME |
| POSITION_ERROR | Target unreachable | Verify limits, HOME |
| COMM_TIMEOUT | Communication lost | Check connections |

## Advanced Diagnostics

### Serial Debug Mode

Enable verbose output for detailed diagnostics:
```
CONFIG SET verbosity 3
```

Verbosity levels:
- 0: Errors only
- 1: Basic status
- 2: Detailed info
- 3: Debug output

### Memory Monitoring

Check system memory health:
```
# Via web interface: Diagnostics tab
# Shows:
- Free heap memory
- Largest free block
- Stack usage per task
- Memory fragmentation
```

### Task Monitoring

Verify all system tasks running:
```
# Expected tasks:
- Motion (Core 0)
- Serial (Core 1)
- Web (Core 1)
- DMX (Core 0)
- Config (Core 1)
```

### Performance Metrics

Monitor timing and performance:
- Motion update rate: Should be 500Hz
- Web update rate: Should be 10Hz
- DMX packet rate: Should be ~44Hz

## Recovery Procedures

### Soft Reset
```
# Via serial
RESTART

# Or power cycle ESP32
```

### Factory Reset
```
CONFIG RESET
# Then reconfigure all parameters
```

### Emergency Recovery

If system completely unresponsive:
1. Disconnect motor power
2. Power cycle ESP32
3. Connect via serial
4. Run diagnostics
5. Re-upload firmware if needed

## Getting Help

### Before Requesting Support

Gather this information:
1. Firmware version (`STATUS` output)
2. Configuration dump (`CONFIG` output)
3. Error messages or codes
4. Steps to reproduce issue
5. Hardware configuration details

### Support Channels

- GitHub Issues: Report bugs and request features
- Documentation: Check guides in `/docs` folder
- Hardware Manual: Review CL57Y driver documentation
- Community Forum: Share experiences and solutions

## Preventive Maintenance

### Daily Checks
- Monitor motor temperature
- Listen for unusual sounds
- Check position accuracy
- Verify limit switch operation

### Weekly Maintenance
- Review error logs
- Test emergency stop
- Check wire connections
- Clean dust from electronics

### Monthly Tasks
- Full system test
- Calibration check
- Update documentation
- Backup configuration

## Safety Reminders

⚠️ **Always**:
- Disconnect power before wiring changes
- Have emergency stop accessible
- Keep hands clear during motion
- Use proper grounding
- Follow electrical safety procedures