#!/bin/bash
# Build POSIX9 test program for PowerPC Mac OS 9

RETRO68_PREFIX="$HOME/Retro68/build/toolchain"
PPC_CC="$RETRO68_PREFIX/bin/powerpc-apple-macos-gcc"
PPC_LD="$RETRO68_PREFIX/bin/powerpc-apple-macos-gcc"
REZ="$RETRO68_PREFIX/bin/Rez"
MAKEAPPL="$RETRO68_PREFIX/bin/MakeAPPL"
MAKE_PEF="$RETRO68_PREFIX/bin/MakePEF"

POSIX9_DIR="$(dirname "$0")/.."
POSIX9_LIB="$POSIX9_DIR/build-ppc/libposix9.a"
POSIX9_INC="$POSIX9_DIR/include"
MAC_STUBS="$POSIX9_DIR/include/mac_stubs"

echo "=== Building POSIX9 Test Program ==="

# Compile test
echo "Compiling posix9_test.c..."
$PPC_CC -O2 -I"$POSIX9_INC" -I"$MAC_STUBS" -c posix9_test.c -o posix9_test.o
if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed"
    exit 1
fi

# Link - no RetroConsole needed since we write to a file
echo "Linking..."
$PPC_LD posix9_test.o "$POSIX9_LIB" -o posix9_test.xcoff
if [ $? -ne 0 ]; then
    echo "ERROR: Linking failed"
    exit 1
fi

# Convert to PEF
echo "Creating PEF..."
$MAKE_PEF posix9_test.xcoff -o posix9_test.pef
if [ $? -ne 0 ]; then
    echo "ERROR: PEF creation failed"
    exit 1
fi

# Create resource file
echo "Creating resources..."
cat > posix9_test.r << 'EOF'
#include "Processes.r"

resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    multiFinderAware,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    isHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    1024 * 1024,    /* preferred size */
    512 * 1024      /* minimum size */
};
EOF

$REZ posix9_test.r -o posix9_test.rsrc 2>/dev/null || true

# Create application
echo "Creating Mac application..."
$MAKEAPPL -o "POSIX9 Test" -c posix9_test.pef -r posix9_test.rsrc 2>/dev/null || \
$MAKEAPPL -o "POSIX9 Test" -c posix9_test.pef

if [ -f "POSIX9 Test" ] || [ -f "POSIX9 Test.bin" ]; then
    echo ""
    echo "=== Build successful! ==="
    ls -la "POSIX9 Test"* 2>/dev/null || ls -la posix9_test.*
    echo ""
    echo "Transfer 'POSIX9 Test' to your Mac OS 9 machine"
else
    echo ""
    echo "Created PEF file (may need manual conversion to app)"
    ls -la posix9_test.*
fi
