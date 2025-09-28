# CL57Y Stepper Driver Wiring Guide - SkullStepperV4

## Overview
This guide describes the wiring configuration for connecting the ESP32-S3 to the CL57Y closed-loop stepper driver using 5V level shifters.

## Hardware Components
- **ESP32-S3 Development Board**
- **CL57Y Closed-Loop Stepper Driver**
- **5V Level Shifters** (integrated on PCB)
- **5V Power Supply** for level shifters and CL57Y positive inputs

## Pin Configuration

### ESP32-S3 GPIO Assignments
| Function | GPIO Pin | Description |
|----------|----------|-------------|
| STEP | GPIO 48 | Step pulse output (open-drain) |
| DIR | GPIO 47 | Direction control (open-drain) |
| ENABLE | GPIO 21 | Motor enable (open-drain) |
| ALARM | GPIO 36 | Position error input from CL57Y |
| LEVEL_ENABLE | GPIO 10 | Enable 5V level shifters (active LOW) |
| LEFT_LIMIT | GPIO 39 | Left limit switch (active low) |
| RIGHT_LIMIT | GPIO 38 | Right limit switch (active low) |

## Wiring Configuration

### Level Shifter Setup
```
ESP32 GPIO 10 → Level Shifter Enable (active LOW)
Level Shifter LV → 3.3V supply
Level Shifter HV → 5V supply
Level Shifter GND → Common ground
```

### CL57Y Stepper Driver Connections
```
CL57Y PP+ (Step Positive) → 5V supply
CL57Y PP- (Step Negative) → Level Shifter HV Output (GPIO 48)

CL57Y DIR+ (Direction Positive) → 5V supply
CL57Y DIR- (Direction Negative) → Level Shifter HV Output (GPIO 47)

CL57Y MF+ (Enable Positive) → 5V supply
CL57Y MF- (Enable Negative) → Level Shifter HV Output (GPIO 21)

CL57Y ALARM+ → ESP32 GPIO 36 (direct connection)
CL57Y ALARM- → Ground
```

## Signal Operation

### Level Shifter Operation
1. **GPIO 10 LOW**: Enables 5V level shifters (normal operation)
2. **GPIO 10 HIGH**: Disables level shifters (shutdown)

### Stepper Control Signals
1. **ESP32 GPIO LOW**: Level shifter outputs 0V → CL57Y optocoupler activates
2. **ESP32 GPIO HIGH/Hi-Z**: Level shifter outputs 5V → CL57Y optocoupler inactive

### Current Flow Path
```
5V Supply → CL57Y Optocoupler LED → Level Shifter HV Output (when GPIO is LOW)
```

## Advantages of This Configuration

### ✅ Benefits
- **Clean 5V signaling** on both sides of optocouplers
- **Better noise immunity** with 5V logic levels
- **Proper current handling** through level shifters
- **No external pull-up resistors required**
- **Compatible with existing open-drain software**

### ✅ Software Compatibility
- **No code changes required** - existing ODStepper open-drain configuration works perfectly
- **Correct signal polarity** - GPIO LOW activates stepper functions
- **Proper timing** - level shifters don't affect signal timing

## Troubleshooting

### Common Issues
1. **Level shifters not enabled**: Check GPIO 10 is LOW during operation
2. **No stepper response**: Verify 5V supply to CL57Y positive inputs
3. **Erratic operation**: Check ground connections between ESP32 and level shifters

### Verification Steps
1. **Power Check**: Measure 5V on CL57Y positive inputs (PP+, DIR+, MF+)
2. **Signal Check**: Verify level shifter outputs switch between 0V and 5V
3. **Enable Check**: Confirm GPIO 10 is LOW during startup
4. **Ground Check**: Ensure common ground between all components

## Safety Notes
- **Power sequencing**: Ensure 5V supply is stable before enabling level shifters
- **Ground isolation**: CL57Y provides optocoupler isolation for noise immunity
- **Current limits**: Level shifters handle the optocoupler LED current (~10-20mA)

---
*Updated: 2025-09-28*
*SkullStepperV4 v4.1.15*