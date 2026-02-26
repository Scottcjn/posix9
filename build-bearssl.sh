#!/bin/bash
# Build BearSSL as a static library for PowerPC Mac OS 9
# Uses Retro68 cross-compiler
#
# BearSSL is compiled in full — the linker only pulls in what's referenced.
# We exclude platform-specific files (x86, POWER8, 64-bit) that won't
# compile on a 32-bit PowerPC G4.

set -e

RETRO68_PREFIX="$HOME/Retro68/build/toolchain"
PPC_CC="$RETRO68_PREFIX/bin/powerpc-apple-macos-gcc"
PPC_AR="$RETRO68_PREFIX/bin/powerpc-apple-macos-ar"

BEARSSL_DIR="$(dirname "$0")/deps/bearssl"
BUILD_DIR="$(dirname "$0")/build-ppc"

if [ ! -f "$PPC_CC" ]; then
    echo "ERROR: Retro68 toolchain not found at $PPC_CC"
    exit 1
fi

if [ ! -d "$BEARSSL_DIR/src" ]; then
    echo "ERROR: BearSSL source not found at $BEARSSL_DIR"
    echo "Run: cd deps && git clone https://www.bearssl.org/git/BearSSL bearssl"
    exit 1
fi

echo "=== Building BearSSL for PowerPC Mac OS 9 ==="

mkdir -p "$BUILD_DIR/bearssl_objs"

# Compiler flags:
#   -O2           : Optimize for size/speed balance
#   -mcpu=750     : Target G3/G4 (safe baseline)
#   -fno-exceptions: No C++ exceptions (pure C anyway)
#   -w            : Suppress warnings (BearSSL is well-tested)
#   -DBR_BE_UNALIGNED=0 : Big-endian but DON'T assume unaligned access is fast
#   -DBR_64=0     : Not a 64-bit architecture
#   -DBR_POWER8=0 : Not POWER8 (no crypto instructions)
#   -DBR_AES_X86NI=0 : No x86 AES-NI
#   -DBR_SSE2=0   : No SSE2
#   -DBR_RDRAND=0 : No RDRAND
#   -DBR_USE_URANDOM=0 : No /dev/urandom on Mac OS 9
#   -DBR_USE_GETENTROPY=0 : No getentropy() on Mac OS 9
#   -DBR_USE_UNIX_TIME=0  : No Unix time() on Mac OS 9
CFLAGS="-O2 -mcpu=750 -fno-exceptions -w"
CFLAGS="$CFLAGS -I$BEARSSL_DIR/inc -I$BEARSSL_DIR/src"
CFLAGS="$CFLAGS -DBR_BE_UNALIGNED=0 -DBR_64=0 -DBR_POWER8=0"
CFLAGS="$CFLAGS -DBR_AES_X86NI=0 -DBR_SSE2=0 -DBR_RDRAND=0"
CFLAGS="$CFLAGS -DBR_USE_URANDOM=0 -DBR_USE_GETENTROPY=0 -DBR_USE_UNIX_TIME=0"
CFLAGS="$CFLAGS -DBR_USE_WIN32_RAND=0 -DBR_USE_WIN32_TIME=0"

# Files to EXCLUDE (platform-specific, won't compile on PPC G4):
EXCLUDE_PATTERNS=(
    # x86 AES-NI intrinsics
    "aes_x86ni"
    # x86 SSE2
    "chacha20_sse2"
    # x86 PCLMUL (carry-less multiply)
    "ghash_pclmul"
    # POWER8 AES/GHASH (needs ISA 2.07 crypto instructions)
    "aes_pwr8"
    "ghash_pwr8"
    # 64-bit specific (uses __int128 or 64-bit multiply)
    "ec_c25519_m62"
    "ec_c25519_m64"
    "ec_p256_m62"
    "ec_p256_m64"
    "i62_modpow2"
    "rsa_i62_"
    # System RNG (uses /dev/urandom or Win32 API)
    "sysrng"
)

should_exclude() {
    local file="$1"
    local basename=$(basename "$file")
    for pattern in "${EXCLUDE_PATTERNS[@]}"; do
        if echo "$basename" | grep -q "$pattern"; then
            return 0  # exclude
        fi
    done
    return 1  # include
}

SUCCESS=0
FAIL=0
SKIP=0
OBJECTS=""

# Compile all BearSSL .c files
for src in $(find "$BEARSSL_DIR/src" -name '*.c' | sort); do
    basename_src=$(basename "$src" .c)
    obj="$BUILD_DIR/bearssl_objs/${basename_src}.o"

    if should_exclude "$src"; then
        SKIP=$((SKIP + 1))
        continue
    fi

    if $PPC_CC $CFLAGS -c "$src" -o "$obj" 2>/dev/null; then
        OBJECTS="$OBJECTS $obj"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "  [FAIL] $src"
        $PPC_CC $CFLAGS -c "$src" -o "$obj" 2>&1 | head -5 | sed 's/^/    /'
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "BearSSL compilation: $SUCCESS compiled, $SKIP skipped, $FAIL failed"

if [ $SUCCESS -eq 0 ]; then
    echo "ERROR: No BearSSL objects compiled!"
    exit 1
fi

# Create static library
echo "Creating libbearssl.a..."
$PPC_AR rcs "$BUILD_DIR/libbearssl.a" $OBJECTS

echo ""
echo "=== BearSSL library built ==="
ls -lh "$BUILD_DIR/libbearssl.a"
echo "Objects: $SUCCESS"
