# Contributing to SkullStepperV4

Thank you for your interest in contributing to SkullStepperV4! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

We are committed to providing a welcoming and inspiring community for all. Please be respectful and constructive in all interactions.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues to avoid duplicates. When creating a bug report, include:

1. **Clear Title**: Descriptive summary of the issue
2. **Environment**: 
   - Firmware version
   - Hardware configuration
   - ESP32-S3 board model
3. **Steps to Reproduce**: Detailed steps to reproduce the issue
4. **Expected Behavior**: What should happen
5. **Actual Behavior**: What actually happens
6. **Additional Context**: Error messages, logs, screenshots

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion, include:

1. **Use Case**: Why is this enhancement needed?
2. **Proposed Solution**: How should it work?
3. **Alternatives Considered**: Other approaches you've thought about
4. **Additional Context**: Mockups, examples, references

### Pull Requests

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Make your changes following the coding standards
4. Test thoroughly
5. Commit your changes (`git commit -m 'Add: Amazing feature for X'`)
6. Push to the branch (`git push origin feature/AmazingFeature`)
7. Open a Pull Request

## Development Setup

### Prerequisites

1. **Arduino CLI** (latest version)
   ```bash
   # Install Arduino CLI
   curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
   ```

2. **ESP32 Board Support**
   ```bash
   arduino-cli core install esp32:esp32
   ```

3. **Required Libraries**
   ```bash
   arduino-cli lib install "ODStepper"
   arduino-cli lib install "FastAccelStepper"
   arduino-cli lib install "ESP32 S3 DMX"
   ```

### Building the Project

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .

# Upload (adjust port)
arduino-cli upload -p /dev/cu.usbmodem11301 --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .

# Monitor serial output
arduino-cli monitor -p /dev/cu.usbmodem11301 -c baudrate=115200
```

## Coding Standards

### General Guidelines

1. **Follow Existing Patterns**: Match the style of surrounding code
2. **Module Isolation**: Modules never directly call functions from other modules
3. **Thread Safety**: Use mutexes for shared data, queues for communication
4. **Memory Safety**: Prefer static allocation, no malloc in real-time tasks
5. **Error Handling**: Always validate inputs and handle errors gracefully

### File Organization

```cpp
// File header (required for all source files)
// ============================================================================
// File: ModuleName.cpp
// Project: SkullStepperV4 - ESP32-S3 Closed-Loop Stepper Control
// Version: 4.1.13
// Date: 2025-01-16
// Author: Your Name
// Description: Brief description of module purpose
// License: MIT
// ============================================================================

// Includes (grouped by type)
#include "ModuleName.h"
#include "GlobalInterface.h"
#include <Arduino.h>
#include <FreeRTOS.h>

// Constants
#define MODULE_CONSTANT 100

// Static variables
static int s_moduleVariable = 0;

// Function implementations
void moduleFunction() {
    // Implementation
}
```

### Naming Conventions

Follow company naming standards:

```cpp
// Classes: PascalCase
class MotionController {
};

// Functions: camelCase
void processCommand(int param);

// Variables: camelCase with descriptive names
int motorPosition = 0;
int targetSpeed = 1000;

// Constants: UPPER_SNAKE_CASE
#define MAX_SPEED 50000
const int DEFAULT_ACCELERATION = 10000;

// Global variables: g_ prefix
SystemStatus g_systemStatus;

// Static variables: s_ prefix
static int s_instanceCount = 0;

// Member variables: m_ prefix
class Motor {
    int m_currentPosition;
};

// Enums: PascalCase with UPPER_SNAKE_CASE values
enum MotionState {
    STATE_IDLE,
    STATE_MOVING,
    STATE_ERROR
};
```

### Core Assignment Rules

```cpp
// Core 0: Real-time operations only
// - Motion control
// - Limit monitoring
// - DMX processing
// - NO Serial.print() in normal operation

// Core 1: Communication and management
// - Serial interface
// - Web server
// - Configuration
// - User interaction

// Pin tasks to cores explicitly
xTaskCreatePinnedToCore(
    taskFunction,
    "TaskName",
    4096,           // Stack size
    NULL,           // Parameters
    10,             // Priority
    NULL,           // Handle
    0);             // Core number
```

### Module Communication

```cpp
// NEVER do this:
// otherModule.doSomething();  // Direct module call

// ALWAYS use queues:
MotionCommand cmd;
cmd.type = MOVE_ABSOLUTE;
cmd.position = 10000;
xQueueSend(g_motionCommandQueue, &cmd, portMAX_DELAY);

// Or protected shared data:
if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(10))) {
    g_systemStatus.position = newPosition;
    xSemaphoreGive(g_statusMutex);
}
```

## Testing Requirements

### Before Submitting

1. **Compile Without Warnings**
   ```bash
   arduino-cli compile --warnings all --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .
   ```

2. **Basic Functionality Test**
   - System boots successfully
   - HOME command works
   - Basic motion commands work
   - Web interface loads
   - No memory leaks

3. **Safety Tests**
   - Limit switches stop motion
   - Emergency stop works
   - Fault recovery via HOME
   - No crashes under stress

4. **Update Documentation**
   - Update relevant .md files
   - Add comments for complex code
   - Update CHANGELOG.md
   - Update version numbers if needed

### Test Protocol

Run the standard test protocol from [TESTING_PROTOCOL.md](../guides/TESTING_PROTOCOL.md):

```bash
# 1. Connect and verify
STATUS

# 2. Home the system
HOME

# 3. Run automated tests
TEST     # Range test
TEST2    # Random position test

# 4. Test limits
# Manually trigger each limit switch

# 5. Test web interface
# Connect to http://192.168.4.1
# Verify all controls work
```

## Documentation

### Code Comments

```cpp
// Single-line comments for simple explanations
int speed = 1000;  // Steps per second

/*
 * Multi-line comments for complex logic
 * Explain WHY, not just WHAT
 * Include any assumptions or limitations
 */
void complexFunction() {
    // Implementation
}

/**
 * @brief Function documentation (Doxygen style)
 * @param param1 Description of parameter
 * @return Description of return value
 */
int documentedFunction(int param1) {
    return param1 * 2;
}
```

### Commit Messages

Follow the company git standards:

```bash
# Format: <type>: <subject>
git commit -m "Add: DMX channel mapping feature"
git commit -m "Fix: Limit switch debouncing issue"
git commit -m "Update: Improve motion acceleration profiles"
git commit -m "Docs: Add hardware setup guide"

# Types:
# Add: New feature or capability
# Fix: Bug fix
# Update: Enhancement to existing feature
# Remove: Removal of feature or code
# Refactor: Code restructuring
# Test: Test additions or changes
# Docs: Documentation only
# Style: Formatting, no code change
```

## Project Structure

When adding new features, maintain the project structure:

```
SkullStepperV4/
├── *.h, *.cpp              # Module files (flat structure for Arduino)
├── docs/
│   ├── user/              # End-user documentation
│   ├── developer/         # Developer documentation
│   ├── guides/            # How-to guides
│   └── design/            # Architecture documents
├── scripts/
│   ├── git/               # Git utilities
│   ├── build/             # Build scripts
│   └── utils/             # Other utilities
└── extras/                # Additional resources
```

## Review Process

### Pull Request Review Criteria

1. **Code Quality**
   - Follows coding standards
   - No compiler warnings
   - Proper error handling
   - Thread-safe implementation

2. **Testing**
   - All tests pass
   - New tests for new features
   - No regression in existing features

3. **Documentation**
   - Code is commented
   - Documentation updated
   - CHANGELOG updated

4. **Safety**
   - No safety compromises
   - Limit switches respected
   - Fault handling maintained

### Review Timeline

- Initial review: Within 3-5 days
- Follow-up reviews: Within 2-3 days
- Small fixes: Within 1-2 days

## Getting Help

### Resources

- [README.md](../../README.md) - Project overview
- [API Reference](api-reference.md) - Complete API documentation
- [Hardware Setup](../user/hardware-setup.md) - Hardware configuration
- [Troubleshooting](../user/troubleshooting.md) - Common issues

### Communication

- **GitHub Issues**: Bug reports and feature requests
- **Pull Requests**: Code contributions
- **Discussions**: General questions and ideas

## License

By contributing to SkullStepperV4, you agree that your contributions will be licensed under the MIT License.

## Recognition

Contributors will be recognized in:
- Git commit history
- CHANGELOG.md for significant contributions
- README.md contributors section (for major contributors)

Thank you for contributing to make SkullStepperV4 better!