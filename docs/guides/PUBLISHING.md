# Publishing SkullStepperV4 to GitHub

This document provides guidance for managing and publishing the SkullStepperV4 project to GitHub.

## Project Overview

SkullStepperV4 is a production-ready ESP32-S3 stepper motor control system with:
- Thread-safe dual-core architecture
- Web and serial interfaces
- DMX512 control integration
- Auto-range homing with safety features
- Persistent configuration storage

## Scripts

### update_git.sh
Automated script for committing and pushing changes:
- Adds all changes to git
- Creates commit with provided message
- Pushes to origin/main

Usage:
```bash
./update_git.sh "Your commit message"
```

### git_restore_tool.sh
Interactive tool for managing git history:
- View recent commits
- Create backup branches
- Restore to previous commits
- Safe recovery options

Usage:
```bash
./git_restore_tool.sh
```

### fix_serial.sh / restore_serial.sh
Utilities for managing serial output during development:
- `fix_serial.sh`: Temporarily disables verbose serial output
- `restore_serial.sh`: Restores original serial output

Usage:
```bash
./fix_serial.sh    # Before testing with other tools
./restore_serial.sh # To restore normal operation
```

### make_executable.sh
Makes all shell scripts executable:

Usage:
```bash
./make_executable.sh
```

## Version Management

Current version: **4.1.0** (Production Ready)

When releasing a new version:
1. Update version in `SkullStepperV4.ino`
2. Update version in `README.md`
3. Update version in `design_docs.md`
4. Create git tag: `git tag v4.1.0`
5. Push tag: `git push origin v4.1.0`

## Pre-flight Checklist

Before publishing updates, ensure:
- [ ] All modules compile without errors
- [ ] Serial interface commands work correctly
- [ ] Web interface loads and functions properly
- [ ] Homing sequence completes successfully
- [ ] Configuration persists across reboots
- [ ] README.md reflects current implementation status
- [ ] design_docs.md is up to date
- [ ] No sensitive information in code (WiFi passwords, etc.)
- [ ] All test commands (TEST, TEST2) work properly

## Development Workflow

### Making Changes
1. **Always read documentation first**
   - Review README.md for current status
   - Check design_docs.md for architecture
   - Understand module interactions

2. **Test thoroughly**
   - Run homing sequence
   - Test serial commands
   - Verify web interface
   - Check limit switches
   - Validate configuration persistence

3. **Update documentation**
   - Update README.md if features added/changed
   - Update design_docs.md for architectural changes
   - Keep inline comments current
   - Document any new commands or parameters

4. **Commit and push**
   ```bash
   ./update_git.sh "Add: New feature description"
   ```

### Commit Message Format
Use descriptive commit messages:
- `Add: New feature or capability`
- `Fix: Bug fix description`
- `Update: Enhancement or improvement`
- `Refactor: Code restructuring`
- `Docs: Documentation updates`
- `Test: Test additions or changes`

Examples:
```bash
./update_git.sh "Add: Percentage-based home position configuration"
./update_git.sh "Fix: Web interface stress test continuous operation"
./update_git.sh "Update: Enhanced limit switch debouncing"
./update_git.sh "Docs: Add MOVEHOME command to help system"
```

## Repository Structure

```
SkullStepperV4/
├── SkullStepperV4.ino      # Main Arduino sketch
├── *.cpp / *.h             # Module implementations
├── README.md               # Project overview and status
├── design_docs.md          # Detailed architecture documentation
├── PUBLISHING.md           # This file
├── ProjectConfig.h         # Build configuration
├── HardwareConfig.h        # Pin assignments
├── Archive/                # Deprecated code
└── *.sh                    # Utility scripts
```

## Module Documentation

Each module should maintain:
- **Header file (.h)**: Public interface documentation
- **Implementation (.cpp)**: Internal documentation
- **Design docs**: Architecture and design decisions
- **README updates**: Feature status and usage

## GitHub Repository Management

### Branches
- `main`: Production-ready code
- `backup-*`: Automatic backups from git_restore_tool
- Feature branches: For major developments

### Releases
1. Ensure all tests pass
2. Update version numbers
3. Create comprehensive release notes
4. Tag release: `git tag -a v4.1.0 -m "Production ready with web interface"`
5. Push: `git push origin v4.1.0`

### Issues and Features
Track using GitHub Issues:
- Bug reports with steps to reproduce
- Feature requests with use cases
- Documentation improvements
- Hardware compatibility notes

## Hardware Documentation

When documenting hardware:
- ESP32-S3 pin assignments (see HardwareConfig.h)
- CL57Y stepper driver connections
- Limit switch wiring with RC filters
- DMX interface connections
- Power supply requirements

## Testing Protocol

Before any release:
1. **Hardware Tests**
   - Verify stepper movement
   - Test both limit switches
   - Check emergency stop
   - Validate homing sequence

2. **Software Tests**
   - All serial commands
   - Web interface on multiple devices
   - Configuration save/load
   - Stress tests (TEST, TEST2)
   - DMX control (if connected)

3. **Integration Tests**
   - Serial + Web simultaneous control
   - Configuration changes via both interfaces
   - Fault recovery procedures
   - Power cycle testing

## Contributing Guidelines

For contributors:
1. Fork the repository
2. Create feature branch
3. Follow existing code style
4. Update documentation
5. Test thoroughly
6. Submit pull request with description

## License

This project is released under the MIT License. Ensure all contributions are compatible.

## Support and Contact

For issues or questions:
- Create GitHub issue with detailed description
- Include version number and hardware setup
- Provide error messages or unexpected behavior
- Attach configuration export if relevant

---

**Remember**: This is a production system controlling physical hardware. Always prioritize safety and thorough testing.
