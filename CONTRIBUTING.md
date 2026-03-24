

:

| Platform | CPU | OS Version | Priority |
|----------|-----|------------|----------|
| PowerMac G3/G4 | PowerPC 750/G4 | Mac OS 8.6 - 9.2.2 | High |
| PowerMac G5 | PowerPC 970 | Mac OS 9.2.2 | High |
| 68K Mac | 68040 | System 7.5 - 8.1 | Medium |

### Testing in Emulators

Use emulators for development testing:

- **SheepShaver** - PowerPC Mac OS 7.5.2 - 9.0.4
- **Basilisk II** - 68K Mac OS 7.x - 8.1
- **QEMU** - Both 68K and PowerPC (more complex setup)

## Submitting Changes

### Pull Request Checklist

Before submitting a PR, ensure:

- [ ] Code compiles without warnings on both PowerPC and 68K
- [ ] New functions have appropriate error handling
- [ ] Tests pass (or new tests added for new features)
- [ ] Documentation updated (README, BUILDING.md, or code comments)
- [ ] Commit messages follow our format
- [ ] No merge conflicts with `main` branch

### Commit Message Format

```
type: brief description (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain WHAT changed and WHY, not HOW (the code shows how).

- Bullet points for multiple changes
- Reference issues: Fixes #123
```

**Types:**
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation changes
- `test:` - Adding or updating tests
- `refactor:` - Code refactoring
- `build:` - Build system changes
- `chore:` - Maintenance tasks

### Review Process

1. PR is reviewed by maintainers
2. Feedback is provided (if any)
3. Changes requested are addressed
4. PR is merged when approved

## Architecture Overview

### POSIX9 Layer Structure

```
┌─────────────────────────────────────────────┐
│  POSIX Application (SSH client, etc.)       │
├─────────────────────────────────────────────┤
│  libposix9.a                                │
│  ├── File I/O (posix9_file.c)              │
│  ├── Directory Ops (posix9_dir.c)          │
│  ├── Path Translation (posix9_path.c)      │
│  ├── Threading (posix9_thread.c)           │
│  ├── Signals (posix9_signal.c)             │
│  └── Sockets (posix9_socket.c)             │
├─────────────────────────────────────────────┤
│  Mac OS Toolbox                             │
│  ├── File Manager (HFS/FSSpec)             │
│  ├── Open Transport (TCP/IP)               │
│  ├── Thread Manager                        │
│  └── Deferred Task Manager                 │
└─────────────────────────────────────────────┘
```

### Path Translation

POSIX9 automatically converts between POSIX and Mac paths:

| POSIX Path | Mac Path | Description |
|------------|----------|-------------|
| `/` | Volume root | Default boot volume |
| `/Volumes/Macintosh HD/Users/scott` | `Macintosh HD:Users:scott` | Absolute path |
| `./foo/bar` | `:foo:bar` | Relative path |
| `../parent` | `::parent` | Parent directory |
| `~/.ssh/config` | `Macintosh HD:Users:scott:.ssh:config` | Home expansion |

### Memory Management

Classic Mac OS uses different memory models:

- **System 7.x**: Uses Memory Manager with Handles
- **Mac OS 8/9**: Supports modern memory allocation

POSIX9 provides `posix9_malloc()` that maps to appropriate Mac OS calls.

## Platform-Specific Guidelines

### PowerPC (Primary Platform)

- Target: Mac OS 7.5.2 through 9.2.2
- Compiler: `powerpc-apple-macos-gcc`
- Alignment: Natural alignment (4-byte for 32-bit)
- Endian: Big-endian

### 68K (Secondary Platform)

- Target: System 7.0 through Mac OS 8.1
- Compiler: `m68k-apple-macos-gcc`
- Alignment: May require packed structures
- Endian: Big-endian

### Code Differences

Use conditional compilation for platform-specific code:

```c
#ifdef TARGET_CPU_PPC
    /* PowerPC-specific code */
    #define POSIX9_CACHE_LINE_SIZE 32
#elif defined(TARGET_CPU_68K)
    /* 68K-specific code */
    #define POSIX9_CACHE_LINE_SIZE 16
#endif
```

## TODO

### High Priority

- [ ] Complete socket implementation (Open Transport integration)
- [ ] Add select() and poll() support
- [ ] Implement fnmatch() and glob()
- [ ] Add termios support for serial ports

### Medium Priority

- [ ] Add more POSIX.1-2001 functions
- [ ] Improve thread cancellation support
- [ ] Add shared memory support
- [ ] Implement POSIX message queues

### Documentation

- [ ] Add API reference documentation
- [ ] Create porting guide for common Unix apps
- [ ] Write troubleshooting guide
- [ ] Add more code examples

## Resources

### Documentation

- [Retro68 Documentation](https://github.com/autc04/Retro68)
- [Inside Macintosh](https://developer.apple.com/library/archive/documentation/mac/pdf/InsideMacintosh.pdf) - Classic Mac OS API reference
- [POSIX.1-2001 Standard](https://pubs.opengroup.org/onlinepubs/007908799/)

### Community

- [Elyan Labs Discord](https://discord.gg/cafc4nDV)
- GitHub Discussions (for this repo)

### Related Projects

- [Retro68](https://github.com/autc04/Retro68) - Cross-compiler for Classic Mac OS
- [SheepShaver](https://sheepshaver.cebix.net/) - PowerPC emulator
- [Basilisk II](https://basilisk.cebix.net/) - 68K emulator

## Questions?

If you have questions:
1. Check existing [GitHub Issues](https://github.com/Scottcjn/posix9/issues)
2. Open a new issue with the `question` label
3. Join the [Elyan Labs Discord](https://discord.gg/cafc4nDV)

---

Thank you for contributing to POSIX9! Your efforts help bring modern Unix capabilities to classic Macintosh computers.
