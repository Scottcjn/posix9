#!/bin/bash
# Build POSIX9 + RustChain Miner for PowerPC Mac OS 9
# Optionally with BearSSL TLS support (USE_TLS=1)
#
# Usage:
#   ./build-ppc.sh            # Build without TLS (plain HTTP)
#   USE_TLS=1 ./build-ppc.sh  # Build with BearSSL TLS (HTTPS)

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
PROJECT_ROOT="$(pwd)"
mkdir -p build-ppc
cd build-ppc

# Compiler flags for Mac OS 9 PowerPC
# Include our headers first, then Mac stubs, then Retro68's headers
CFLAGS="-O2 -fno-exceptions -I../include -I../include/mac_stubs -w"

# ============================================================
# Step 1: Build BearSSL (if USE_TLS is enabled)
# ============================================================

if [ "${USE_TLS:-0}" = "1" ]; then
    echo ""
    echo "=== TLS Mode: Building BearSSL ==="

    if [ ! -d "$PROJECT_ROOT/deps/bearssl/src" ]; then
        echo "ERROR: BearSSL not found. Run:"
        echo "  cd deps && git clone https://www.bearssl.org/git/BearSSL bearssl"
        exit 1
    fi

    # Run dedicated BearSSL build script
    if [ ! -f "$PROJECT_ROOT/build-ppc/libbearssl.a" ]; then
        bash "$PROJECT_ROOT/build-bearssl.sh"
        if [ $? -ne 0 ]; then
            echo "ERROR: BearSSL build failed!"
            exit 1
        fi
    else
        echo "  libbearssl.a already exists, skipping rebuild"
        echo "  (delete build-ppc/libbearssl.a to force rebuild)"
    fi

    # Add TLS flags
    CFLAGS="$CFLAGS -DUSE_TLS=1"
    CFLAGS="$CFLAGS -I$PROJECT_ROOT/deps/bearssl/inc"
    TLS_LIBS="libbearssl.a"
    echo ""
else
    echo ""
    echo "  (Plain HTTP mode — set USE_TLS=1 for HTTPS)"
    TLS_LIBS=""
fi

# ============================================================
# Step 2: Build POSIX9 library
# ============================================================

echo "Compiling POSIX9 library..."

SKIP_FILES=""

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

# ============================================================
# Step 3: Build RustChain Miner
# ============================================================

echo ""
echo "=== Building RustChain Miner ==="

MINER_OBJECTS=""

# Compile the miner
echo -n "  rustchain_miner_posix9.c ... "
if $PPC_CC $CFLAGS -I../miner -c ../miner/rustchain_miner_posix9.c -o rustchain_miner_posix9.o 2>/dev/null; then
    echo "OK"
    MINER_OBJECTS="$MINER_OBJECTS rustchain_miner_posix9.o"
else
    echo "FAILED"
    $PPC_CC $CFLAGS -I../miner -c ../miner/rustchain_miner_posix9.c -o rustchain_miner_posix9.o 2>&1 | head -20
    exit 1
fi

# Compile TLS wrapper if TLS mode
if [ "${USE_TLS:-0}" = "1" ]; then
    echo -n "  tls_wrapper.c ... "
    TLS_CFLAGS="$CFLAGS -I$PROJECT_ROOT/deps/bearssl/inc -I../miner"
    if $PPC_CC $TLS_CFLAGS -c ../miner/tls_wrapper.c -o tls_wrapper.o 2>/dev/null; then
        echo "OK"
        MINER_OBJECTS="$MINER_OBJECTS tls_wrapper.o"
    else
        echo "FAILED"
        $PPC_CC $TLS_CFLAGS -c ../miner/tls_wrapper.c -o tls_wrapper.o 2>&1 | head -20
        exit 1
    fi
fi

# Link everything
echo ""
echo "Linking RustChain Miner..."

# Retro68 Mac OS import libraries (resolve Mac Toolbox symbols)
IMPORT_LIBDIR="$RETRO68_PREFIX/universal/libppc"
IMPORT_LIBS="-L$IMPORT_LIBDIR -lInterfaceLib -lOpenTransportLib -lOpenTptInternetLib -lThreadsLib"

LINK_LIBS="libposix9.a"
if [ -n "$TLS_LIBS" ]; then
    LINK_LIBS="$TLS_LIBS $LINK_LIBS"
fi

$PPC_CC -o RustChainMiner $MINER_OBJECTS $LINK_LIBS $IMPORT_LIBS 2>&1
if [ $? -eq 0 ]; then
    echo ""
    echo "========================================="
    if [ "${USE_TLS:-0}" = "1" ]; then
        echo "  RustChain Miner built (TLS enabled!)"
    else
        echo "  RustChain Miner built (plain HTTP)"
    fi
    echo "========================================="
    ls -lh RustChainMiner
else
    echo "ERROR: Link failed!"
    $PPC_CC -o RustChainMiner $MINER_OBJECTS $LINK_LIBS $IMPORT_LIBS 2>&1
    exit 1
fi

# ============================================================
# Step 4: Create PEF (Preferred Executable Format) via MakePEF
# ============================================================

MAKEPEF="$RETRO68_PREFIX/bin/powerpc-apple-macos-makepef"
if [ -f "$MAKEPEF" ]; then
    echo ""
    echo "Creating Mac OS 9 PEF..."
    $MAKEPEF RustChainMiner -o "RustChain Miner" 2>/dev/null
    if [ -f "RustChain Miner" ]; then
        ls -lh "RustChain Miner"
    fi
fi

echo ""
echo "Build complete!"
if [ "${USE_TLS:-0}" = "1" ]; then
    echo "  Mode:   HTTPS (TLS 1.2 via BearSSL)"
    echo "  Server: $NODE_HOST:443"
else
    echo "  Mode:   HTTP (plain)"
    echo "  Server: $NODE_HOST:8088"
fi
