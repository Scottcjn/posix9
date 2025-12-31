# Building POSIX9

Detailed build instructions for POSIX9 on different platforms.

## Prerequisites

### 1. Install Retro68 Cross-Compiler

POSIX9 uses the [Retro68](https://github.com/autc04/Retro68) cross-compiler to build for Classic Mac OS.

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake libgmp-dev libmpfr-dev libmpc-dev \
    libboost-all-dev bison texinfo ruby

# Clone Retro68
git clone --recursive https://github.com/autc04/Retro68.git
cd Retro68

# Build toolchain (this takes 1-2 hours)
mkdir build && cd build

# Option 1: PowerPC only (faster, ~30 min)
../build-toolchain.bash --ppc-only

# Option 2: Full build (68K + PowerPC, ~90 min)
../build-toolchain.bash
```

The toolchain will be installed to `~/Retro68/build/toolchain/`.

### 2. Verify Toolchain

```bash
# Check PowerPC compiler
~/Retro68/build/toolchain/bin/powerpc-apple-macos-gcc --version

# Check 68K compiler (if built)
~/Retro68/build/toolchain/bin/m68k-apple-macos-gcc --version
```

## Building POSIX9

### PowerPC Build (Mac OS 7.5.2 - 9.2.2)

```bash
cd posix9
./build-ppc.sh
```

**Output:**
```
=== Building POSIX9 for PowerPC Mac OS 9 ===
Compiling POSIX9 library...
  posix9_dir.c -> posix9_dir.o ... OK
  posix9_file.c -> posix9_file.o ... OK
  posix9_path.c -> posix9_path.o ... OK
  posix9_signal.c -> posix9_signal.o ... OK
  posix9_thread.c -> posix9_thread.o ... OK
  [SKIP] posix9_misc.c (known type conflicts)
  [SKIP] posix9_socket.c (known type conflicts)

Compilation results: 5 succeeded, 0 failed

Creating libposix9.a from successful objects...

=== POSIX9 library built ===
-rw-r--r-- 1 user user 51234 Dec 30 12:00 libposix9.a
```

### 68K Build (System 7.0 - 8.1)

```bash
cd posix9
./build-68k.sh
```

**Output:**
```
=== Building POSIX9 for 68K Mac ===
...
Creating libposix9-68k.a from successful objects...
```

## Building Applications with POSIX9

### Method 1: Direct Compilation

```bash
# Set paths
export RETRO68="$HOME/Retro68/build/toolchain"
export POSIX9_DIR="/path/to/posix9"

# PowerPC
$RETRO68/bin/powerpc-apple-macos-gcc \
    -O2 \
    -I"$POSIX9_DIR/include" \
    -I"$POSIX9_DIR/include/mac_stubs" \
    -c myapp.c -o myapp.o

$RETRO68/bin/powerpc-apple-macos-gcc \
    myapp.o "$POSIX9_DIR/build-ppc/libposix9.a" \
    -o myapp.xcoff

# Convert to Mac executable
$RETRO68/bin/MakePEF myapp.xcoff -o "My App.pef"

# 68K
$RETRO68/bin/m68k-apple-macos-gcc \
    -O2 -m68020 \
    -I"$POSIX9_DIR/include" \
    -I"$POSIX9_DIR/include/mac_stubs" \
    -c myapp.c -o myapp.o

$RETRO68/bin/m68k-apple-macos-gcc \
    myapp.o "$POSIX9_DIR/build-68k/libposix9-68k.a" \
    -o myapp
```

### Method 2: CMake Integration

Create a `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.12)
project(MyApp C)

# Include POSIX9
set(POSIX9_DIR "/path/to/posix9")
include_directories(${POSIX9_DIR}/include)
include_directories(${POSIX9_DIR}/include/mac_stubs)

add_executable(MyApp myapp.c)

# Link POSIX9
if(CMAKE_SYSTEM_NAME STREQUAL "Retro68")
    target_link_libraries(MyApp ${POSIX9_DIR}/build-ppc/libposix9.a)
endif()
```

Build:
```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$HOME/Retro68/build/toolchain/powerpc-apple-macos/cmake/retro68.toolchain.cmake
make
```

## Target-Specific Notes

### PowerPC (Mac OS 8/9)

- **Compiler**: `powerpc-apple-macos-gcc`
- **Output**: XCOFF → PEF via `MakePEF`
- **Min OS**: Mac OS 7.5.2 (CFM required)
- **Recommended**: Mac OS 8.6+ for Open Transport sockets

**Key flags:**
```
-O2                 Optimization
-fno-exceptions     Disable C++ exceptions (smaller code)
-w                  Suppress warnings
```

### 68K (System 7/8)

- **Compiler**: `m68k-apple-macos-gcc`
- **Output**: Direct Mac application
- **Min CPU**: 68020 (System 7 requirement)
- **Min OS**: System 7.0

**Key flags:**
```
-m68020             Target 68020+ CPU
-O2                 Optimization
-fno-exceptions     Disable C++ exceptions
```

### Differences Between Platforms

| Feature | PowerPC | 68K |
|---------|---------|-----|
| Code format | PEF (CFM) | CODE resources |
| Min CPU | 601 | 68020 |
| Threading | Thread Manager | Thread Manager |
| Networking | Open Transport | MacTCP/OT |
| Max code size | ~16MB | ~32KB/segment |

## Troubleshooting

### "Undefined reference to PBGetCatInfoSync"

On PowerPC, `PBGetCatInfoSync` is an external function imported from InterfaceLib, not an inline trap. Don't redefine it in headers.

**Fix:** Remove any `#define PBGetCatInfoSync` from your code.

### "Conflicting types for 'time_t'"

Newlib (Retro68's C library) defines `time_t`, `pid_t`, etc. Use guards:

```c
#ifndef _TIME_T_DECLARED
typedef long time_t;
#endif
```

### "TMTask has no member 'tmWakeUp'"

Different Mac OS versions have different `TMTask` structures. Use `memset()` instead of field initialization:

```c
TMTask task;
memset(&task, 0, sizeof(task));
task.tmAddr = myCallback;
```

### Build takes too long

- Use `--ppc-only` when building Retro68 if you only need PowerPC
- Use `-j$(nproc)` for parallel compilation
- Pre-build Retro68 once and keep the toolchain

## Testing

### Build Test Program

```bash
cd test
./build-test.sh
```

### Transfer to Mac

1. **Network (AFP/AppleTalk)**
   ```bash
   # On Linux with netatalk
   sudo apt install netatalk
   # Copy to AFP share
   ```

2. **Floppy Disk**
   - Format HFS floppy on Mac
   - Mount on Linux: `sudo mount -t hfs /dev/fd0 /mnt`
   - Copy files

3. **Serial (Zterm)**
   - Use XMODEM/ZMODEM protocol
   - 115200 baud recommended

4. **Compact Flash Adapter**
   - Format CF card as HFS
   - Use PCMCIA-CF adapter on Mac

### Run on Mac

1. Double-click the application
2. Check `POSIX9 Test Log` file on desktop
3. Verify all tests passed

## Build Artifacts

After building:

```
posix9/
├── build-ppc/
│   ├── libposix9.a          # PowerPC static library
│   ├── posix9_dir.o
│   ├── posix9_file.o
│   ├── posix9_path.o
│   ├── posix9_signal.o
│   └── posix9_thread.o
├── build-68k/
│   ├── libposix9-68k.a      # 68K static library
│   └── *.o
└── test/
    ├── posix9_test.xcoff    # PowerPC test (XCOFF)
    ├── posix9_test.pef      # PowerPC test (PEF)
    └── POSIX9 Test          # Mac application
```
