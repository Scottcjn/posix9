[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0) [![Mac OS 9](https://img.shields.io/badge/Mac_OS-9-lightgrey)](https://github.com/Scottcjn/posix9) [![POSIX](https://img.shields.io/badge/POSIX-Compatible-green)](https://github.com/Scottcjn/posix9)

# POSIX9 - POSIX Compatibility Layer for Classic Mac OS

A shim library providing POSIX-compatible APIs on top of Classic Mac OS Toolbox calls, enabling modern Unix software to compile and run on vintage Macintosh computers.

## Supported Platforms

| Platform | CPU | OS Version | Toolchain | Status |
|----------|-----|------------|-----------|--------|
| **68K Mac** | 68020+ | System 7.0 - 8.1 | Retro68 (m68k) | Supported |
| **PowerPC Mac** | G3/G4/G5 | Mac OS 7.5.2 - 9.2.2 | Retro68 (ppc) | Primary |

## Features

- **File I/O**: `open`, `read`, `write`, `close`, `lseek`, `stat`, `fstat`, `unlink`
- **Directories**: `opendir`, `readdir`, `closedir`, `mkdir`, `rmdir`, `chdir`, `getcwd`
- **Path Translation**: Automatic POSIX ↔ Mac path conversion
- **Sockets**: BSD socket API via Open Transport (Mac OS 8.6+)
- **Threads**: POSIX threads via Thread Manager
- **Signals**: Emulated signal handling via Deferred Tasks
- **Time**: `time`, `localtime`, `strftime`, `gettimeofday`
- **Environment**: `getenv`, `setenv`, `unsetenv`

## Architecture

```
┌─────────────────────────────────────┐
│  POSIX Application (SSH, etc.)      │
├─────────────────────────────────────┤
│  libposix9.a                        │
│  ├── posix9_file.c    (file I/O)   │
│  ├── posix9_dir.c     (directories)│
│  ├── posix9_path.c    (path xlat)  │
│  ├── posix9_thread.c  (pthreads)   │
│  ├── posix9_signal.c  (signals)    │
│  ├── posix9_socket.c  (networking) │
│  └── posix9_misc.c    (utilities)  │
├─────────────────────────────────────┤
│  Mac OS Toolbox                     │
│  ├── File Manager (FSSpec/HFS)     │
│  ├── Open Transport (TCP/IP)       │
│  ├── Thread Manager                │
│  └── Time Manager                  │
└─────────────────────────────────────┘
```

## Path Translation

POSIX9 automatically converts between POSIX and Mac paths:

| POSIX Path | Mac Path |
|------------|----------|
| `/` | Volume root (default volume) |
| `/Volumes/Macintosh HD/Users/scott` | `Macintosh HD:Users:scott` |
| `./foo/bar` | `:foo:bar` |
| `../parent` | `::parent` |

## Building with Retro68

### Prerequisites

1. **Retro68 Cross-Compiler**: https://github.com/autc04/Retro68

```bash
# Clone Retro68
git clone https://github.com/autc04/Retro68.git
cd Retro68

# Build for both 68K and PowerPC
mkdir build && cd build
../build-toolchain.bash --ppc-only   # PowerPC only (faster)
# OR
../build-toolchain.bash               # Full build (68K + PPC)
```

### Building POSIX9

#### PowerPC Build (Mac OS 7.5.2 - 9.2.2)

```bash
cd posix9
./build-ppc.sh
```

This creates `build-ppc/libposix9.a` (~50KB) with the following modules:
- `posix9_dir.o` - Directory operations
- `posix9_file.o` - File I/O
- `posix9_path.o` - Path translation
- `posix9_signal.o` - Signal emulation
- `posix9_thread.o` - POSIX threads

#### 68K Build (System 7.0 - 8.1)

```bash
cd posix9
./build-68k.sh
```

### Building Your Application

```bash
# Set environment variables
export RETRO68_PREFIX="$HOME/Retro68/build/toolchain"
export POSIX9_LIB="/path/to/posix9/build-ppc/libposix9.a"
export POSIX9_INC="/path/to/posix9/include"

# Compile (PowerPC)
$RETRO68_PREFIX/bin/powerpc-apple-macos-gcc \
    -O2 \
    -I"$POSIX9_INC" \
    -I"$POSIX9_INC/mac_stubs" \
    -c myapp.c -o myapp.o

# Link
$RETRO68_PREFIX/bin/powerpc-apple-macos-gcc \
    myapp.o "$POSIX9_LIB" -o myapp.xcoff

# Convert to PEF (Mac executable)
$RETRO68_PREFIX/bin/MakePEF myapp.xcoff -o myapp.pef
```

## API Reference

### File I/O

```c
#include "posix9.h"

int fd = open("/path/to/file", O_RDONLY);
char buf[1024];
ssize_t n = read(fd, buf, sizeof(buf));
close(fd);

// Create file
fd = open("/new/file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
write(fd, "Hello Mac OS 9!
", 16);
close(fd);
```

### Directories

```c
DIR *dir = opendir("/");
struct dirent *ent;
while ((ent = readdir(dir)) != NULL) {
    printf("%s
", ent->d_name);
}
closedir(dir);

mkdir("/new/directory", 0755);
chdir("/some/path");
getcwd(buf, sizeof(buf));
```

### Path Translation

```c
char mac_path[256];
posix9_path_to_mac("/Volumes/Macintosh HD/file.txt", mac_path, sizeof(mac_path));
// Result: "Macintosh HD:file.txt"

char posix_path[256];
posix9_path_from_mac("Macintosh HD:Users:scott", posix_path, sizeof(posix_path));
// Result: "/Volumes/Macintosh HD/Users/scott"
```

### Sockets (Mac OS 8.6+ with Open Transport)

```c
int sock = socket(AF_INET, SOCK_STREAM, 0);

struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(22);
addr.sin_addr.s_addr = inet_addr("192.168.1.1");

connect(sock, (struct sockaddr*)&addr, sizeof(addr));
send(sock, "Hello
", 6, 0);
close(sock);
```

### Threads

```c
#include "posix9.h"

void *thread_func(void *arg) {
    // Thread code
    return NULL;
}

pthread_t tid;
pthread_create(&tid, NULL, thread_func, NULL);
pthread_join(tid, NULL);
```

## Limitations

### No Process Model
Mac OS 9 has no true process model. These functions are stubbed:
- `fork()` - Returns -1 with `ENOSYS`
- `exec*()` - Launches separate app (doesn't replace current process)
- `getpid()` - Always returns 1

### Networking
- Sockets require **Open Transport** (Mac OS 8.6+)
- System 7 has no TCP/IP stack by default
- MacTCP is not supported

### Signals
Signals are emulated via polling. Some behaviors differ from Unix:
- No async signal delivery (polled in event loop)
- `SIGKILL`/`SIGSTOP` not implemented

### File Systems
- No symbolic links (HFS/HFS+ limitation)
- 31-character filename limit (HFS)
- No file permissions (mode is ignored)

## Directory Structure

```
posix9/
├── include/
│   ├── posix9.h              # Main header (include this)
│   ├── posix9/
│   │   ├── types.h           # POSIX types
│   │   ├── errno.h           # Error codes
│   │   ├── socket.h          # Socket definitions
│   │   ├── pthread.h         # Thread definitions
│   │   ├── signal.h          # Signal definitions
│   │   ├── time.h            # Time definitions
│   │   └── unistd.h          # Misc definitions
│   └── mac_stubs/
│       ├── Multiverse.h      # Retro68 Mac API
│       ├── MacCompat.h       # Missing Mac definitions
│       ├── Timer.h           # Time Manager stubs
│       ├── Threads.h         # Thread Manager stubs
│       └── OpenTransport.h   # OT stubs
├── src/
│   ├── posix9_file.c         # File operations
│   ├── posix9_dir.c          # Directory operations
│   ├── posix9_path.c         # Path translation
│   ├── posix9_thread.c       # POSIX threads
│   ├── posix9_signal.c       # Signal emulation
│   ├── posix9_socket.c       # BSD sockets
│   └── posix9_misc.c         # Misc utilities
├── test/
│   ├── posix9_test.c         # Test program
│   └── build-test.sh         # Test build script
├── build-ppc.sh              # PowerPC build script
├── build-68k.sh              # 68K build script
└── README.md                 # This file
```

## Testing on Real Hardware

1. Build the test program:
```bash
cd test
./build-test.sh
```

2. Transfer `POSIX9 Test` to your Mac via:
   - Floppy disk
   - Network (AppleTalk/AFP)
   - Serial connection (Zterm)
   - Compact Flash adapter

3. Run on Mac OS 9 - creates `POSIX9 Test Log` file with results

## Related Projects

- **GUSI** - Grand Unified Socket Interface by Matthias Neeracher
- **MachTen** - Full BSD on Mac OS (commercial, discontinued)
- **A/UX** - Apple's Unix for 68K Macs
- **MPW POSIX** - Partial POSIX in Macintosh Programmer's Workshop
- **Retro68** - Cross-compiler for Classic Mac OS

## Target Applications

POSIX9 was created to port:
- **Dropbear SSH** - Lightweight SSH server
- **curl/wget** - HTTP clients
- **Python 2.x** - Scripting language
- **Various Unix utilities**

## License

MIT License

## Contributing

Contributions welcome! Areas needing work:
- [ ] Complete `posix9_socket.c` Open Transport implementation
- [ ] Fix `posix9_misc.c` type conflicts with newlib
- [ ] Add 68K-specific optimizations
- [ ] Test on real System 7 hardware
- [ ] Port more Unix software

## Status

| Module | PPC | 68K | Notes |
|--------|-----|-----|-------|
| posix9_file.c | OK | OK | File operations |
| posix9_dir.c | OK | OK | Directory operations |
| posix9_path.c | OK | OK | Path translation |
| posix9_signal.c | OK | OK | Signal emulation |
| posix9_thread.c | OK | OK | POSIX threads |
| posix9_socket.c | WIP | WIP | Needs OT constants |
| posix9_misc.c | WIP | WIP | Type conflicts |

**Current library size**: ~50KB (5 modules)
