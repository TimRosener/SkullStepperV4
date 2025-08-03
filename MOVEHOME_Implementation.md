# MOVEHOME Command Implementation Summary

## What We Added

### 1. **SystemConfig Changes**
- Changed `homePosition` (fixed value) to `homePositionPercent` (configurable percentage)
- Default: 50% (center of range)
- Range: 0-100%
- Allows user to configure where the system returns to after homing

### 2. **New Commands**
- `MOVEHOME` or `GOTOHOME` - Move to the configured home position
- `CONFIG SET homePositionPercent <value>` - Set the home position (0-100%)
- `CONFIG RESET homePositionPercent` - Reset to default (50%)

### 3. **How It Works**
1. System must be homed first (position limits established)
2. MOVEHOME calculates target position: `minPos + (range * homePositionPercent / 100)`
3. Moves to that calculated position
4. Shows the target position and percentage in the output

### 4. **Example Usage**
```
HOME                              # Establish position limits first
CONFIG SET homePositionPercent 25 # Set home to 25% of range
MOVEHOME                          # Move to 25% position
CONFIG SET homePositionPercent 75 # Change to 75% of range  
MOVEHOME                          # Move to 75% position
```

### 5. **Safety Features**
- Requires system to be homed before use
- Validates percentage is 0-100%
- Shows clear error messages if preconditions not met
- Respects all motion limits and safety systems

### 6. **Configuration Persistence**
- Home position percentage is saved to flash memory
- Survives power cycles
- Can be reset to default with CONFIG RESET command

## Testing Recommendations

1. **Basic Test**:
   ```
   HOME
   STATUS                          # Check limits established
   CONFIG SET homePositionPercent 10
   MOVEHOME                        # Should go to 10% position
   CONFIG SET homePositionPercent 90
   MOVEHOME                        # Should go to 90% position
   ```

2. **Edge Cases**:
   ```
   CONFIG SET homePositionPercent 0   # Far left
   MOVEHOME
   CONFIG SET homePositionPercent 100 # Far right
   MOVEHOME
   CONFIG SET homePositionPercent 50  # Center
   MOVEHOME
   ```

3. **Persistence Test**:
   - Set a custom home position percentage
   - Power cycle the system
   - Check that the setting persisted
   - MOVEHOME should go to the saved position

## Integration with DMX
The MOVEHOME position serves as a known reference point that the system can return to when DMX control is not active or as a startup position.
