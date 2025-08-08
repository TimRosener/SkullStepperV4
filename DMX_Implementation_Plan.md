# DMX Implementation Plan for SkullStepperV4
## ESP32S3DMX Library Integration
### Version: 2.1 - Final Design
### Created: 2025-02-02
### Status: Ready for Implementation

---

## Overview

This document outlines the plan to implement DMX control for SkullStepperV4 using the ESP32S3DMX library. The implementation will provide professional-grade DMX control with configurable channel offset and multi-parameter control.

## DMX Channel Design

### Channel Layout (Base + Offset)

| Channel | Function | Range | Mapping | Notes |
|---------|----------|-------|---------|-------|
| Base+0 | Position MSB | 0-255 | 0-100% of range | Coarse position (8-bit) |
| Base+1 | Position LSB | 0-255 | Fine position | 16-bit precision when combined with Ch0 |
| Base+2 | Acceleration | 0-255 | 0-100% of max accel | Dynamic acceleration limit |
| Base+3 | Speed | 0-255 | 0-100% of max speed | Dynamic speed limit |
| Base+4 | Mode Control | 0-255 | 3 modes | Stop/DMX/Home |

### Mode Control Detail (Channel Base+4)
- **0-84**: STOP mode (motor holds position, ignores position channel)
- **85-170**: DMX CONTROL mode (follows position channel)
- **171-255**: FORCE HOME mode (initiates homing sequence)

### Design Decisions Made
1. **Default base channel**: 1 ✓
2. **Position smoothing**: Not needed ✓
3. **Signal loss behavior**: Hold position ✓
4. **16-bit precision**: Available using channels 0 & 1 ✓

## Implementation Phases

### Phase 1: Core DMX Infrastructure ✅ COMPLETE
- [x] Create new DMXReceiver implementation with ESP32S3DMX
- [x] Add configurable base channel (default: 1)
- [x] Implement 5-channel reading
- [x] Create channel value caching
- [x] Add signal loss detection with hold behavior

**Key Features:**
```cpp
class DMXReceiver {
    uint16_t baseChannel;      // Configurable offset (1-508)
    uint8_t channelCache[5];   // Cache for our 5 channels
    bool dmxConnected;
    uint32_t lastPacketTime;
    ESP32S3DMX dmx;            // DMX receiver instance
};
```

### Phase 2: Channel Processing ✅ COMPLETE
- [x] Implement position mapping (8-bit and 16-bit modes)
- [x] Implement acceleration scaling
- [x] Implement speed scaling
- [x] Implement mode detection with hysteresis
- [x] Add signal timeout handling

**Mode Detection with Hysteresis:**
```cpp
enum DMXMode {
    MODE_STOP,      // 0-84
    MODE_CONTROL,   // 85-170
    MODE_HOME       // 171-255
};
// Add ±5 hysteresis to prevent mode flickering
```

### Phase 3: System Integration ✅ COMPLETE
- [x] Add DMX configuration to SystemConfig
- [x] Integrate with StepperController
- [x] Handle mode transitions properly
- [x] Implement dynamic speed/accel limits
- [x] Add safety checks

**Configuration Structure:**
```cpp
struct DMXConfig {
    uint16_t baseChannel;        // 1-508 (default: 1)
    bool enabled;                // DMX enabled flag
    uint8_t modeHysteresis;      // Mode change threshold (default: 5)
    uint32_t signalTimeout;      // ms before timeout (default: 1000)
    bool use16BitPosition;       // Enable 16-bit position mode
};
```

### Phase 4: Motion Integration ✅ COMPLETE
- [x] Connect position channel to motion commands
- [x] Apply acceleration limits dynamically
- [x] Apply speed limits dynamically
- [x] Handle mode transitions smoothly
- [x] Implement hold-on-signal-loss behavior
- [x] Fixed channel cache corruption issues
- [x] Handle DMX zero values gracefully

**Motion Command Flow:**
```
DMX Position → Scale to % → Check Mode → Apply Limits → Send to StepperController
```

### Phase 5: Web Interface Updates
- [ ] Add DMX configuration tab
- [ ] Show current channel values
- [ ] Display mode and status
- [ ] Allow base channel configuration
- [ ] Show signal quality/rate
- [ ] Add 16-bit mode toggle

**Web Interface Elements:**
- Base channel input (1-508)
- Channel value display (real-time)
- Mode indicator with visual feedback
- Signal status (connected/rate/timeout)
- 16-bit position mode checkbox
- Enable/disable toggle

### Phase 6: Serial Interface Updates
- [ ] Add DMX STATUS command
- [ ] Add DMX CONFIG commands
- [ ] Add channel monitor mode
- [ ] Add DMX simulation for testing
- [ ] Add diagnostic commands

**New Serial Commands:**
```
DMX STATUS              - Show all DMX info
DMX CONFIG CHANNEL n    - Set base channel
DMX CONFIG 16BIT on/off - Enable 16-bit mode
DMX MONITOR             - Live channel display
DMX SIMULATE ch val     - Simulate DMX value
DMX ENABLE/DISABLE      - Control DMX
```

### Phase 7: 16-bit Position Implementation
- [ ] Add 16-bit position mode toggle
- [ ] Implement MSB/LSB combination
- [ ] Test precision improvements
- [ ] Update web interface for mode selection
- [ ] Document 16-bit mode usage

### Phase 8: Testing & Validation
- [ ] Test all channel mappings
- [ ] Verify mode transitions
- [ ] Test signal loss hold behavior
- [ ] Validate 16-bit precision
- [ ] Test with various DMX consoles
- [ ] Long-term stability test

## Implementation Details

### Channel Value Processing

```cpp
// Position processing (8-bit or 16-bit)
float position;
if (dmxConfig.use16BitPosition) {
    // 16-bit: Combine MSB and LSB
    uint16_t pos16 = (dmxCache[0] << 8) | dmxCache[1];
    position = (pos16 / 65535.0) * 100.0;
} else {
    // 8-bit: Use MSB only
    position = (dmxCache[0] / 255.0) * 100.0;
}

// Acceleration (0-255 → % of configured max)
float accelPercent = (dmxCache[2] / 255.0) * 100.0;
float actualAccel = (systemConfig.maxAccel * accelPercent) / 100.0;

// Speed (0-255 → % of configured max)
float speedPercent = (dmxCache[3] / 255.0) * 100.0;
float actualSpeed = (systemConfig.maxSpeed * speedPercent) / 100.0;

// Mode (with hysteresis)
DMXMode mode = detectMode(dmxCache[4]);
```

### State Machine for Mode Control

```
STOP Mode:
- Ignore position changes
- Hold current position
- Still apply E-stop if needed

CONTROL Mode:
- Follow position channel
- Apply speed/accel from channels
- Normal operation

HOME Mode:
- Trigger homing once
- DMX is then COMPLETELY IGNORED during homing
- Mode can return to STOP immediately (momentary trigger)
- After homing completes, DMX processing resumes
- Mode resets to STOP to prevent re-triggering

HOMING IN PROGRESS State:
- ALL DMX input ignored
- Prevents any DMX commands from interfering
- Ensures homing completes without interruption
- Automatically exits when homing finishes
```

### Signal Loss Handling

```cpp
void handleSignalLoss() {
    if (millis() - lastPacketTime > dmxConfig.signalTimeout) {
        // Signal lost - hold current position
        dmxConnected = false;
        // Do NOT send new motion commands
        // Motor will hold at last position
    }
}
```

### Safety Considerations

1. **Signal Loss**: Hold position on timeout
2. **Mode Changes**: Smooth transitions, no jerky movements
3. **Limit Override**: DMX cannot override safety limits
4. **E-Stop Priority**: E-stop always takes precedence
5. **Homing Safety**: Can't home while in motion
6. **Homing Required States**: 
   - When system is not homed (on boot or after configuration reset)
   - When limit fault is active (after E-STOP or unexpected limit hit)
   - DMX position control is disabled in these states
   - DMX HOME command (mode 171-255) can still initiate homing
   - This ensures DMX can always trigger homing when needed
7. **Homing Sequence Protection**:
   - Once homing starts, ALL DMX input is ignored
   - DMX mode can return to STOP immediately after triggering HOME
   - Prevents re-triggering or interruption of homing
   - DMX processing automatically resumes after homing completes

## Timeline Estimate

- Phase 1: 2 hours (DMX infrastructure)
- Phase 2: 2 hours (channel processing)
- Phase 3: 2 hours (system integration)
- Phase 4: 2 hours (motion integration)
- Phase 5: 2 hours (web interface)
- Phase 6: 1 hour (serial interface)
- Phase 7: 1 hour (16-bit implementation)
- Phase 8: 2 hours (testing)

**Total: ~14 hours of development**

## Success Criteria

- [ ] All 5 channels working correctly (position MSB/LSB adjacent)
- [ ] Smooth mode transitions
- [ ] Configurable base channel
- [ ] Signal loss = hold position
- [ ] 16-bit position mode functional
- [ ] No conflicts with existing features
- [ ] Professional-grade reliability

## Risk Mitigation

1. **Channel Conflicts**: Validate base channel + 4 ≤ 512
2. **Mode Flickering**: Implement hysteresis
3. **Signal Loss**: Clear hold behavior
4. **Safety**: Never override limits or E-stop
5. **Performance**: Cache channel values

## Next Steps

1. Begin Phase 1 implementation
2. Create DMXReceiver with ESP32S3DMX
3. Test basic channel reading
4. Proceed with integration

---

## Progress Tracking

### Session 1 (2025-02-02)
- [x] Created initial plan
- [x] Redesigned for new requirements
- [x] Finalized channel design
- [x] Improved channel layout (Position LSB moved to channel 1)
- [x] Implemented Phase 1: Core DMX Infrastructure
  - ESP32S3DMX library integration
  - Core 0 task for real-time DMX processing
  - 5-channel cache system
  - Signal timeout detection
  - Thread-safe status updates
  - Helper functions for channel processing

### Session 2 (2025-02-02)
- [x] Implemented Phase 2: Channel Processing
  - Position mapping with 8/16-bit support
  - Speed/acceleration scaling from DMX values
  - Mode detection with hysteresis
- [x] Implemented Phase 3: System Integration
  - Full StepperController integration
  - Dynamic parameter application
- [x] Implemented Phase 4: Motion Integration
  - DMX controls position, speed, and acceleration
  - Fixed StepperController to apply motion profile parameters
  - Smooth mode transitions
  - Signal loss handling

### Session 3
- [ ] Implement Phase 5-6: Interface updates
- [ ] Add DMX monitoring and configuration commands

---

**Implementation Ready**: All design decisions made. Ready to start coding!

## Dynamic Speed/Acceleration Control

### Implementation Details (v4.1.6)

DMX now supports dynamic speed and acceleration updates during motion without modifying the stepper library:

1. **Change Detection**: Monitors speed (channel 3) and acceleration (channel 2) for changes > 2 units
2. **Smart Updates**: Only sends updates when:
   - Position changes (as before)
   - Speed/accel changes while motor is moving
   - Prevents unnecessary commands when stationary
3. **Smooth Transitions**: Sends new MOVE_ABSOLUTE commands with same target but updated parameters
4. **Console Feedback**: Logs show "[Speed Changed]" or "[Accel Changed]" tags

```cpp
// Tracks last values to detect changes
static uint8_t lastSpeedValue = 0;
static uint8_t lastAccelValue = 0;

// Detects changes with threshold to prevent jitter
bool speedChanged = (abs(channels[CH_SPEED] - lastSpeedValue) > 2);
bool accelChanged = (abs(channels[CH_ACCELERATION] - lastAccelValue) > 2);

// Only updates if moving or position changing
bool needsUpdate = positionChanged || (isMoving && (speedChanged || accelChanged));
```

This allows real-time performance adjustments during shows without stopping motion.

## Recent Bug Fixes (v4.1.10)

### DMX Channel Cache Corruption Fix

Fixed issue where DMX values would randomly drop to zero:

1. **Root Cause**: Channel cache was being updated even when no new DMX packets arrived
2. **Solution**: Only update cache when receiving valid new packets
3. **Detection**: Added warnings when all channels suddenly go to zero

```cpp
// OLD: Updated cache every cycle
if (dmxConnected) {
    updateChannelCache();  // Could read stale/corrupted data
    processDMXChannels();
}

// NEW: Only use cached values from valid packets
if (dmxConnected) {
    processDMXChannels();  // Uses last valid cache
}
```

### DMX Zero Value Handling

Improved handling when DMX channels send zero values:

1. **Problem**: Zero speed/accel values resulted in minimum 10 steps/sec movement
2. **Solution**: Use system default values when channels are zero
3. **Benefit**: Maintains smooth operation even with problematic DMX sources

```cpp
// When speed channel is 0, use system default
if (channels[CH_SPEED] == 0) {
    actualSpeed = config->defaultProfile.maxSpeed;
} else {
    actualSpeed = (config->defaultProfile.maxSpeed * speedPercent) / 100.0f;
}
```

### Enhanced Debug Output

- Shows all 5 DMX channels including mode
- Detects stuck LSB in 16-bit mode
- Tracks channel wake-up from zero
- Consolidated task alive messages
- Cleaner message flow
