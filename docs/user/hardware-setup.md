# Hardware Setup Guide - SkullStepperV4

## Overview

This guide covers the physical hardware setup for the SkullStepperV4 stepper control system, including wiring, connections, and initial configuration.

## Required Components

### Core Components
- **ESP32-S3 Development Board** (WROOM-1 module recommended)
- **CL57Y Closed-Loop Stepper Driver** or compatible
- **NEMA 23/24 Stepper Motor** (matching driver specifications)
- **24-48VDC Power Supply** (sized for motor current requirements)
- **5V Power Supply** (for ESP32 if not USB powered)

### Safety Components
- **2x Limit Switches** (Normally Closed type)
- **Emergency Stop Button** (optional but recommended)
- **10kΩ Resistors** (2x for external pull-ups if needed)
- **100nF Capacitors** (2x for switch debouncing)

### DMX Components (Optional)
- **RS485 Transceiver Module** (MAX485 or similar)
- **5-pin XLR Connector** or RJ45 for DMX
- **120Ω Termination Resistor**

### Miscellaneous
- **Connecting Wires** (22-18 AWG for signals, 14-16 AWG for power)
- **Terminal Blocks** or connectors
- **Project Enclosure** (metal recommended for EMI shielding)

## Wiring Diagram

### ESP32-S3 Pin Connections

```
ESP32-S3 Pin    Function          Connect To
────────────    ─────────         ──────────
GPIO 7          STEP              CL57Y PUL+ (via optocoupler)
GPIO 15         DIR               CL57Y DIR+ (via optocoupler)
GPIO 16         ENABLE            CL57Y ENA+ (via optocoupler)
GPIO 8          ALARM             CL57Y ALM+ (via optocoupler)
GPIO 17         LEFT_LIMIT        Left Limit Switch
GPIO 18         RIGHT_LIMIT       Right Limit Switch
GPIO 4          DMX_RX            RS485 Module RO
GPIO 5          DMX_TX            RS485 Module DI (if using)
GPIO 6          DMX_ENABLE        RS485 Module DE/RE
3.3V            Power             Board power
GND             Ground            Common ground
```

## Step-by-Step Setup

### 1. Power Supply Preparation

#### ESP32 Power
1. Connect 5V power supply to ESP32 VIN pin (or use USB power)
2. Ensure common ground between ESP32 and motor driver
3. Add 100μF bulk capacitor near ESP32 power input

#### Motor Power
1. Connect 24-48VDC supply to CL57Y driver power inputs
2. Ensure power supply rated for motor current + 20% overhead
3. Use appropriate wire gauge for motor current (typically 14-16 AWG)

### 2. Stepper Driver Connections

#### Control Signals
```
CL57Y Terminal    ESP32 Connection      Notes
──────────────    ────────────────      ─────
PUL+              GPIO 7                Step signal
PUL-              GND                   Common ground
DIR+              GPIO 15               Direction signal
DIR-              GND                   Common ground
ENA+              GPIO 16               Enable (HIGH = enabled)
ENA-              GND                   Common ground
ALM+              GPIO 8                Alarm output
ALM-              GND                   Common ground
```

**Important**: CL57Y inputs are optically isolated. No level shifting required.

#### Motor Connections
1. Connect motor phases to driver terminals:
   - A+/A- to motor coil 1
   - B+/B- to motor coil 2
2. Follow motor datasheet for correct phase identification
3. Use shielded cable for motor wiring to reduce EMI

### 3. Limit Switch Installation

#### Physical Mounting
1. Mount limit switches at mechanical travel limits
2. Ensure solid mounting that won't shift during operation
3. Leave 2-3mm clearance for switch activation
4. Test mechanical activation before wiring

#### Electrical Connection
```
Limit Switch Wiring (Normally Closed Configuration)

        ┌─────────────┐
        │ Limit Switch│
        │             │
        │   C    NC   │
        └───┬────┬────┘
            │    │
            │    └──── GND
            │
            └──────────┤├──────── GPIO 17/18
                      100nF
                     to GND
```

**Wiring Notes**:
- Use shielded cable for limit switch wiring
- Add 100nF capacitor at ESP32 end for hardware debouncing
- Internal pull-ups are enabled in software (10kΩ to 3.3V)

### 4. DMX Interface (Optional)

#### RS485 Module Connection
```
RS485 Module      ESP32          Function
────────────      ─────          ────────
VCC               3.3V           Power
GND               GND            Ground
RO                GPIO 4         Receive data
DI                GPIO 5         Transmit data (if needed)
DE/RE             GPIO 6         Direction control
```

#### DMX Connector Wiring
```
5-Pin XLR         RS485 Module
─────────         ────────────
Pin 1 (GND)       GND
Pin 2 (Data-)     A-
Pin 3 (Data+)     B+
Pin 4 (NC)        Not connected
Pin 5 (NC)        Not connected
```

**Termination**: Add 120Ω resistor between A- and B+ at the end of DMX chain.

## Initial Configuration

### 1. Driver DIP Switch Settings

Configure CL57Y driver DIP switches for your motor:

```
Switch    Setting         Function
──────    ───────         ────────
SW1-3     Motor Current   Set according to motor specs
SW4       ON              Enable internal control
SW5-6     Microstepping   Recommend 1600 steps/rev
SW7       OFF             Disable auto-current reduction
SW8       ON              Enable alarm output
```

Refer to CL57Y manual for detailed DIP switch configurations.

### 2. First Power-On Sequence

1. **Verify all connections** before applying power
2. **Apply ESP32 power first** (USB or 5V supply)
3. **Wait for ESP32 boot** (blue LED should be on)
4. **Apply motor power** to driver
5. **Check driver LED status**:
   - Green = Normal
   - Red = Fault condition
6. **Connect via Serial** (115200 baud) or WiFi

### 3. Software Configuration

#### Serial Connection
1. Open serial terminal (115200 baud)
2. Type `STATUS` to verify connection
3. Run `HOME` command to detect range
4. Configure parameters as needed

#### WiFi Connection
1. Connect to WiFi network: "SkullStepper"
2. Open browser to: http://192.168.4.1
3. Use web interface for configuration

## Safety Checks

### Before First Motion

- [ ] All wires properly connected and secured
- [ ] Power supplies correct voltage
- [ ] Motor shaft free to rotate
- [ ] Limit switches properly positioned
- [ ] Emergency stop accessible (if installed)
- [ ] No loose wires or components
- [ ] Proper grounding established

### Test Sequence

1. **Limit Switch Test**
   - Manually activate each limit switch
   - Verify system detects activation
   - Check that motion stops immediately

2. **Homing Test**
   - Run `HOME` command
   - System should move to find limits
   - Verify range detection completes

3. **Motion Test**
   - Start with slow movements
   - Test small position changes first
   - Gradually increase speed and distance

## Troubleshooting

### Common Issues

#### Motor Doesn't Move
- Check ENABLE signal (should be HIGH)
- Verify driver power and LED status
- Check motor wiring continuity
- Ensure driver DIP switches correct

#### Erratic Motion
- Check step/dir signal connections
- Verify common ground between ESP32 and driver
- Add shielding to motor cables
- Reduce acceleration/speed settings

#### Limit Switches Not Working
- Verify switch type (must be NC)
- Check wiring continuity
- Test with multimeter (should read 0Ω when not activated)
- Ensure pull-ups enabled in software

#### Driver Alarm Active
- Check motor connections
- Verify motor current settings
- Ensure mechanical load not excessive
- Check for position error (binding)

## EMI Considerations

### Noise Reduction
1. Use shielded cables for motor wiring
2. Keep motor cables away from signal wires
3. Add ferrite cores to power cables
4. Use metal enclosure with proper grounding
5. Separate motor ground from logic ground
6. Connect grounds at single point only

### Best Practices
- Route high-current wires separately
- Minimize wire lengths where possible
- Use twisted pair for differential signals
- Add bypass capacitors near ICs
- Ensure solid ground connections

## Maintenance

### Regular Checks
- Inspect wire connections monthly
- Check for loose terminals
- Verify limit switch operation
- Clean dust from electronics
- Check motor temperature during operation

### Preventive Maintenance
- Re-torque terminal connections annually
- Replace limit switches if worn
- Check cable flexibility and strain relief
- Update firmware as needed
- Document any modifications

## Support

For additional help:
- Review [Troubleshooting Guide](troubleshooting.md)
- Check [Serial Interface Manual](../guides/SerialInterface_Manual.md)
- Visit [Web Interface Guide](../guides/WebInterface_Guide.md)
- Open issue on GitHub repository

## Safety Warning

⚠️ **HIGH VOLTAGE PRESENT** ⚠️
- Motor power supply is 24-48VDC
- Can cause injury or equipment damage
- Always disconnect power before wiring changes
- Use proper safety equipment
- Follow local electrical codes