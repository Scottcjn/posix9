#!/bin/bash
# Build POSIX9 for 68K Mac (System 7.0 - 8.1)
# Requires Retro68 with 68K support

RETRO68_PREFIX="$HOME/Retro68/build/toolchain"
M68K_CC="$RETRO68_PREFIX/bin/m68k-apple-macos-gcc"
M68K_AR="$RETRO68_PREFIX/bin/m68k-apple-macos-ar"

# Check if toolchain exists
if [ ! -f "$M68K_CC" ]; then
    echo "ERROR: 68K toolchain not found at $M68K_CC"
    echo "Build Retro68 with 68K support:"
    echo "  cd Retro68 && mkdir build && cd build"
    echo "  ../build-toolchain.bash"
    exit 1
fi

echo "=== Building POSIX9 for 68K Mac ==="

cd "$(dirname "$0")"
mkdir -p build-68k
cd build-68k

# Compiler flags for 68K
# -m68020 for 68020+ (System 7 minimum)
# Include our headers first, then Mac stubs
CFLAGS="-O2 -m68020 -fno-exceptions -I../include -I../include/mac_stubs -w"

echo "Compiling POSIX9 library for 68K..."

# Files with complex conflicts to fix later
SKIP_FILES="posix9_misc.c posix9_socket.c"

SUCCESS_COUNT=0
FAIL_COUNT=0
OBJECTS=""

for src in ../src/posix9_*.c; do
    basename_src=$(basename "$src")
    obj=$(basename "$src" .c).o

    # Check if this file should be skipped
    if echo "$SKIP_FILES" | grep -q "$basename_src"; then
        echo "  [SKIP] $basename_src (known type conflicts)"
        continue
    fi

    echo -n "  $basename_src -> $obj ... "
    if $M68K_CC $CFLAGS -c "$src" -o "$obj" 2>/dev/null; then
        echo "OK"
        OBJECTS="$OBJECTS $obj"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        echo "FAILED"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        # Try again with verbose output to see what's wrong
        echo "    Errors:"
        $M68K_CC $CFLAGS -c "$src" -o "$obj" 2>&1 | head -10 | sed 's/^/      /'
    fi
done

echo ""
echo "Compilation results: $SUCCESS_COUNT succeeded, $FAIL_COUNT failed"

if [ -n "$OBJECTS" ]; then
    echo ""
    echo "Creating libposix9-68k.a from successful objects..."
    $M68K_AR rcs libposix9-68k.a $OBJECTS

    echo ""
    echo "=== POSIX9 68K library built ==="
    ls -la libposix9-68k.a
    echo ""
    echo "Objects included:"
    $M68K_AR t libposix9-68k.a
else
    echo ""
    echo "ERROR: No objects compiled successfully!"
    exit 1
fi

echo ""
echo "To use in your application:"
echo "  export POSIX9_LIB_68K=$(pwd)/libposix9-68k.a"
echo "  export POSIX9_INCLUDE=$(dirname $(pwd))/include"
