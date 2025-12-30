# POSIX9 - POSIX Compatibility Layer for Mac OS 9

A shim library providing POSIX-compatible APIs on top of Mac OS 9 Toolbox calls.

## Goal

Enable modern Unix tools (like SSH, Python, etc.) to compile and run on Mac OS 9 by providing:
- POSIX file I/O (`open`, `read`, `write`, `close`, `lseek`, `stat`)
- Directory operations (`opendir`, `readdir`, `closedir`, `mkdir`)
- Socket networking via Open Transport
- Threading via Thread Manager / MP Services
- Signal emulation via Deferred Tasks

## Target Platform

- Mac OS 9.0 - 9.2.2
- PowerPC (G3/G4)
- CodeWarrior Pro or Retro68 toolchain

## Architecture

```
┌─────────────────────────────────────┐
│  POSIX Application                  │
├─────────────────────────────────────┤
│  posix9 shim library                │
│  ├── posix9_file.c    (file I/O)   │  ✅ Done
│  ├── posix9_dir.c     (directories)│  ✅ Done
│  ├── posix9_socket.c  (networking) │  ✅ Done
│  ├── posix9_thread.c  (pthreads)   │  ✅ Done
│  ├── posix9_signal.c  (signals)    │  🚧 TODO
│  └── posix9_path.c    (path xlat)  │  ✅ Done
├─────────────────────────────────────┤
│  Mac OS 9 Toolbox                   │
│  ├── File Manager (FSSpec)          │
│  ├── Open Transport                 │
│  ├── Thread Manager / MP Services   │
│  └── Notification Manager           │
└─────────────────────────────────────┘
```

## Path Translation

| POSIX | Mac OS 9 |
|-------|----------|
| `/` (root) | Volume root |
| `/Volumes/Macintosh HD/foo` | `Macintosh HD:foo` |
| `/Users/scott/file.txt` | `Macintosh HD:Users:scott:file.txt` |
| `./relative/path` | `:relative:path` |

## Building

### With Retro68
```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/retro68/m68k-apple-macos/cmake/retro68.toolchain.cmake
make
```

### With CodeWarrior
Import the `.mcp` project file.

## Prior Art

- **MachTen** - Full BSD on Mac OS (commercial, discontinued)
- **GUSI** - Grand Unified Socket Interface by Matthias Neeracher
- **A/UX** - Apple's Unix for 68k Macs
- **MPW POSIX** - Partial POSIX in Macintosh Programmer's Workshop

## License

MIT

## Status

✅ **POSIX Foundation Complete** - Ready for SSH server port!

### What's Done
- File I/O (open/read/write/close/stat)
- Directory operations (opendir/readdir/mkdir)
- Path translation (POSIX ↔ Mac paths)
- BSD sockets (via Open Transport)
- POSIX threads (via Thread Manager)
- Signals (emulated with polling)
- Time functions (time/localtime/strftime)
- Environment (getenv/setenv)
- Misc utilities (sleep, random, etc.)

### In Progress
- Dropbear SSH server port
- OS 9 installer package
