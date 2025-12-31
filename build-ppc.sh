#!/bin/bash
# Build POSIX9 for PowerPC Mac OS 9
# Run this after Retro68 build completes

RETRO68_PREFIX="$HOME/Retro68/build/toolchain"
PPC_CC="$RETRO68_PREFIX/bin/powerpc-apple-macos-gcc"
PPC_AR="$RETRO68_PREFIX/bin/powerpc-apple-macos-ar"

# Check if toolchain exists
if [ ! -f "$PPC_CC" ]; then
    echo "ERROR: PowerPC toolchain not found at $PPC_CC"
    echo "Wait for Retro68 build to complete!"
    exit 1
fi

echo "=== Building POSIX9 for PowerPC Mac OS 9 ==="

cd "$(dirname "$0")"
mkdir -p build-ppc
cd build-ppc

# Compiler flags for Mac OS 9 PowerPC
# Include our headers first, then Mac stubs, then Retro68's headers
CFLAGS="-O2 -fno-exceptions -I../include -I../include/mac_stubs -w"

echo "Compiling POSIX9 library..."

# Files with complex conflicts to fix later
# - posix9_misc.c: struct timespec/timeval redefinition with newlib
# - posix9_socket.c: Open Transport stub needs more constants
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
    if $PPC_CC $CFLAGS -c "$src" -o "$obj" 2>/dev/null; then
        echo "OK"
        OBJECTS="$OBJECTS $obj"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        echo "FAILED"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        # Try again with verbose output to see what's wrong
        echo "    Errors:"
        $PPC_CC $CFLAGS -c "$src" -o "$obj" 2>&1 | head -10 | sed 's/^/      /'
    fi
done

echo ""
echo "Compilation results: $SUCCESS_COUNT succeeded, $FAIL_COUNT failed"

if [ -n "$OBJECTS" ]; then
    echo ""
    echo "Creating libposix9.a from successful objects..."
    $PPC_AR rcs libposix9.a $OBJECTS

    echo ""
    echo "=== POSIX9 library built ==="
    ls -la libposix9.a
    echo ""
    echo "Objects included:"
    $PPC_AR t libposix9.a
else
    echo ""
    echo "ERROR: No objects compiled successfully!"
    exit 1
fi

echo ""
echo "To use in Dropbear build:"
echo "  export POSIX9_LIB=$(pwd)/libposix9.a"
echo "  export POSIX9_INCLUDE=$(dirname $(pwd))/include"
