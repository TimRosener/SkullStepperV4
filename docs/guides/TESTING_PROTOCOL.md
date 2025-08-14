# SkullStepperV4 Testing Protocol

This document outlines the comprehensive testing procedures for the SkullStepperV4 system.

## Quick Test Checklist

For rapid validation before commits:

- [ ] System boots without errors
- [ ] Serial prompt appears (`skull>`)
- [ ] HOME command completes successfully
- [ ] MOVE command works after homing
- [ ] Web interface loads at 192.168.4.1
- [ ] Position updates in real-time

## Complete Testing Protocol

### 1. Initial Power-On Tests

#### Serial Communication
- [ ] Connect via USB at 115200 baud
- [ ] Verify version string shows "4.1.0"
- [ ] Check all initialization steps show "✓"
- [ ] Confirm `skull>` prompt appears

#### System Status
- [ ] Run `STATUS` command
- [ ] Verify system state is "READY"
- [ ] Check uptime is counting
- [ ] Confirm "Not homed" status

### 2. Configuration Tests

#### View Configuration
- [ ] Run `CONFIG` - verify all parameters display
- [ ] Run `PARAMS` - check parameter ranges
- [ ] Verify default values are sensible

#### Modify Configuration
- [ ] `CONFIG SET maxSpeed 2000` - verify acceptance
- [ ] `CONFIG SET acceleration 1000` - verify acceptance
- [ ] `CONFIG SET homePositionPercent 25` - verify acceptance
- [ ] Restart system - verify settings persist

#### Reset Configuration
- [ ] `CONFIG RESET maxSpeed` - verify returns to default
- [ ] `CONFIG RESET` - verify factory reset warning
- [ ] Confirm factory reset with 'Y'

### 3. Homing Tests

#### Basic Homing
- [ ] Run `HOME` command
- [ ] Verify moves to left limit first
- [ ] Verify backs off and marks as 0
- [ ] Verify finds right limit
- [ ] Verify sets position limits
- [ ] Check final position at home percentage

#### Homing Interruption
- [ ] Start `HOME` command
- [ ] Press any key to interrupt
- [ ] Verify homing stops cleanly
- [ ] Verify system remains unhomed

### 4. Motion Control Tests

#### Basic Movement
- [ ] `MOVE 100` - verify smooth motion
- [ ] `MOVE 0` - verify return to zero
- [ ] `MOVEHOME` - verify moves to home position
- [ ] Watch for smooth acceleration/deceleration

#### Motion Limits
- [ ] Try `MOVE -100` - verify rejection
- [ ] Try `MOVE 999999` - verify clamping to max
- [ ] Verify software limits enforced

#### Emergency Stop
- [ ] Start long move
- [ ] Run `STOP` - verify deceleration
- [ ] Start another move
- [ ] Run `ESTOP` - verify immediate stop

### 5. Limit Switch Tests

#### Manual Trigger
- [ ] During motion, trigger left limit
- [ ] Verify immediate stop
- [ ] Verify fault state requires homing
- [ ] Repeat for right limit

#### Debounce Test
- [ ] Rapidly tap limit switch
- [ ] Verify no false triggers
- [ ] Check clean state transitions

### 6. Web Interface Tests

#### Connection
- [ ] Connect to "SkullStepper" WiFi
- [ ] Browse to http://192.168.4.1
- [ ] Verify page loads completely

#### Status Display
- [ ] Check position updates in real-time
- [ ] Verify state changes reflected
- [ ] Monitor limit indicators
- [ ] Check system info panel

#### Motion Control
- [ ] Click HOME button - verify homing
- [ ] Use position slider - verify movement
- [ ] Click STOP - verify stop
- [ ] Click "Move to Home" - verify movement

#### Configuration
- [ ] Adjust speed slider - verify immediate effect
- [ ] Adjust acceleration - test with movement
- [ ] Change home position - verify persistence

#### Stress Tests
- [ ] Click TEST - verify continuous motion
- [ ] Let run for 10+ cycles
- [ ] Click STOP - verify clean stop
- [ ] Click TEST2 - verify 10 random moves

### 7. Stress Testing

#### Serial Stress Test
- [ ] Run `TEST` command
- [ ] Monitor for 50+ cycles
- [ ] Check no position drift
- [ ] Verify smooth motion throughout
- [ ] Press key to stop

#### Random Position Test
- [ ] Run `TEST2` or `RANDOMTEST`
- [ ] Verify 10 random positions
- [ ] Check all positions within range
- [ ] Monitor for accurate positioning

### 8. Safety Tests

#### Limit Fault Recovery
- [ ] Trigger limit fault
- [ ] Try `MOVE` - verify rejection
- [ ] Run `HOME` - verify clears fault
- [ ] Verify normal operation restored

#### Power Cycle
- [ ] Note current position
- [ ] Power off system
- [ ] Power on system
- [ ] Verify configuration retained
- [ ] Verify requires homing

### 9. Interface Tests

#### Serial Modes
- [ ] `ECHO OFF` - verify no echo
- [ ] `ECHO ON` - verify echo restored
- [ ] `VERBOSE 0` - minimal output
- [ ] `VERBOSE 3` - maximum output
- [ ] `JSON ON` - verify JSON responses
- [ ] `JSON OFF` - verify human readable

#### Help System
- [ ] Run `HELP` - verify comprehensive
- [ ] Check command descriptions
- [ ] Verify examples included

### 10. Integration Tests

#### Simultaneous Control
- [ ] Open web interface
- [ ] Keep serial connected
- [ ] Move via web - verify serial shows updates
- [ ] Move via serial - verify web updates
- [ ] Change config via web - verify in serial

#### Error Conditions
- [ ] Send invalid commands
- [ ] Verify clear error messages
- [ ] Check system remains stable
- [ ] Test boundary conditions

## Performance Benchmarks

Record these metrics for regression testing:

- [ ] Homing completion time: _____ seconds
- [ ] Max reliable speed achieved: _____ steps/sec
- [ ] Position accuracy at 1000 steps: ± _____ steps
- [ ] Web interface response time: _____ ms
- [ ] Stress test 100 cycles time: _____ minutes

## Known Good Configuration

Default working configuration for reference:
- maxSpeed: 1000 steps/sec
- acceleration: 500 steps/sec²
- homingSpeed: 940 steps/sec
- homePositionPercent: 50%
- emergencyDeceleration: 10000 steps/sec²

## Troubleshooting Tests

If issues encountered:

### Motion Problems
- [ ] Check mechanical coupling tight
- [ ] Verify stepper driver connections
- [ ] Test with reduced speed/acceleration
- [ ] Check power supply voltage

### Communication Issues
- [ ] Verify baud rate 115200
- [ ] Check USB cable quality
- [ ] Try different serial terminal
- [ ] Restart serial monitor

### Web Interface Issues
- [ ] Confirm WiFi "SkullStepper" visible
- [ ] Try different browser
- [ ] Check JavaScript console for errors
- [ ] Verify WebSocket connection

## Sign-Off

Testing performed by: _______________________

Date: _______________________

Version tested: 4.1.0

All tests passed: [ ] Yes [ ] No

Notes:
_________________________________________
_________________________________________
_________________________________________
