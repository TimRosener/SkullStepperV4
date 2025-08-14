# DMX Debug Analysis

## Problem Summary

The DMX motion control is not working correctly. The main issues are:

1. **Speed and Acceleration channels are reading 0** despite sliders being at full
2. **Motion control is erratic** with slow movement even when speed should be high
3. **Channel values appear stuck** and not updating properly

## Debug Output Analysis

From your serial output:
```
[DMX] Speed channel is 0, using system default: 9900
[DMX] Acceleration channel is 0, using system default: 20000
[DMX] Pos: 12423 (63.9%) Spd: 9900 Acc: 20000 | Current: 12423 | Moving: NO | DMX[163,154,0,0,113]
```

This shows:
- Position MSB: 163
- Position LSB: 154  
- Speed: 0 (PROBLEM!)
- Acceleration: 0 (PROBLEM!)
- Mode: 113 (CONTROL mode - correct)

## Root Causes

### 1. Channel Mapping Mismatch
The DMX receiver expects:
- Channel 0: Position MSB
- Channel 1: Position LSB
- Channel 2: Acceleration
- Channel 3: Speed
- Channel 4: Mode

But your DMX console might be sending speed/acceleration on different channels.

### 2. Base Channel Offset Issue
The system uses a configurable base channel (default: 1). If your console is sending on channels 1-5 but the system is reading from a different offset, you'll get wrong values.

### 3. DMX Console Configuration
Some DMX consoles:
- Start at channel 0 instead of 1
- Have gaps between channels
- Send 16-bit values differently
- Don't update all channels simultaneously

## Recommended Fixes

### Fix 1: Add DMX Channel Monitoring
Add a diagnostic command to show ALL DMX channels being received, not just the 5 we're using.

### Fix 2: Make Channel Mapping Configurable
Instead of fixed channel offsets, allow remapping:
```cpp
struct DMXChannelMap {
    uint8_t positionMSB;    // Which channel has position MSB
    uint8_t positionLSB;    // Which channel has position LSB  
    uint8_t speed;          // Which channel has speed
    uint8_t acceleration;   // Which channel has acceleration
    uint8_t mode;           // Which channel has mode
};
```

### Fix 3: Add Raw DMX Monitor Mode
Show the first 20 channels continuously to see what's actually being sent:
```
DMX Monitor: [1:163][2:154][3:0][4:0][5:113][6:0][7:0]...
```

### Fix 4: Remove Zero-Value Default Behavior
Instead of using system defaults when DMX is 0, treat 0 as a valid value (stop or minimum speed).

## Immediate Debugging Steps

1. **Check your DMX console channel assignments**
   - What channels are position, speed, acceleration actually on?
   - Is the console 0-based or 1-based?

2. **Try different base channel values**
   ```
   DMX CONFIG CHANNEL 0   // If console is 0-based
   DMX CONFIG CHANNEL 2   // If there's an offset
   ```

3. **Monitor raw DMX values**
   - We need to see what all channels are sending
   - Not just the 5 we think we're using

4. **Test with simple values**
   - Set position to 50% (128)
   - Set speed to 50% (128)
   - Set acceleration to 50% (128)
   - See what the debug shows

## Code Issues Found

1. **Channel Cache Update Logic**: The cache is only updated when new packets arrive, but the values might be wrong due to channel mapping.

2. **Speed/Acceleration Scaling**: When channels are 0, it uses defaults. This masks the real problem.

3. **Debug Output**: Shows processed values, not raw DMX input for all channels.

## Next Steps

1. Add a `DMX MONITOR` command that shows raw values for channels 1-20
2. Make channel mapping configurable via serial commands
3. Add better diagnostics for channel assignment issues
4. Consider auto-detection of channel layout
