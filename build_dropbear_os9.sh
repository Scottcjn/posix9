#!/bin/bash
# Build Dropbear SSH for Mac OS 9 with POSIX9
set -e

export PATH="$HOME/Retro68/build/toolchain/bin:$PATH"
PPC_CC="powerpc-apple-macos-gcc"
PPC_AR="powerpc-apple-macos-ar"

POSIX9_DIR="/home/scott/projects/posix9"
DROPBEAR_DIR="$POSIX9_DIR/dropbear-2024.86"
BUILD_DIR="$POSIX9_DIR/build-dropbear-os9"

CFLAGS="-O2 -DDROPBEAR_MACOS9 -DRETRO68_BUILD -w"
INCLUDES="-I$DROPBEAR_DIR -I$DROPBEAR_DIR/libtommath -I$DROPBEAR_DIR/libtomcrypt/src/headers -I$POSIX9_DIR/include"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "=== Step 1: Building libtommath ==="
mkdir -p libtommath
SUCCESS=0
FAIL=0
for src in "$DROPBEAR_DIR"/libtommath/bn_*.c; do
    obj="libtommath/$(basename ${src%.c}.o)"
    if $PPC_CC $CFLAGS -I"$DROPBEAR_DIR/libtommath" -c "$src" -o "$obj" 2>/dev/null; then
        ((SUCCESS++))
    else
        ((FAIL++))
    fi
done
echo "  libtommath: $SUCCESS compiled, $FAIL failed"
$PPC_AR rcs libtommath/libtommath.a libtommath/*.o 2>/dev/null || true

echo ""
echo "=== Step 2: Building libtomcrypt ==="
mkdir -p libtomcrypt
LTCFLAGS="$CFLAGS -DLTC_NO_FILE -DLTC_NO_PROTOTYPES -DARGTYPE=3 -DLTM_DESC"
SUCCESS=0
FAIL=0
for src in "$DROPBEAR_DIR"/libtomcrypt/src/*/*.c "$DROPBEAR_DIR"/libtomcrypt/src/*/*/*.c; do
    [ -f "$src" ] || continue
    obj="libtomcrypt/$(basename ${src%.c}.o)"
    if $PPC_CC $LTCFLAGS -I"$DROPBEAR_DIR/libtommath" -I"$DROPBEAR_DIR/libtomcrypt/src/headers" -c "$src" -o "$obj" 2>/dev/null; then
        ((SUCCESS++))
    else
        ((FAIL++))
    fi
done
echo "  libtomcrypt: $SUCCESS compiled, $FAIL failed"
$PPC_AR rcs libtomcrypt/libtomcrypt.a libtomcrypt/*.o 2>/dev/null || true

echo ""
echo "=== Step 3: Building POSIX9 ==="
mkdir -p posix9
SUCCESS=0
FAIL=0
for src in "$POSIX9_DIR"/src/posix9_*.c; do
    obj="posix9/$(basename ${src%.c}.o)"
    if $PPC_CC $CFLAGS $INCLUDES -c "$src" -o "$obj" 2>/dev/null; then
        ((SUCCESS++))
    else
        ((FAIL++))
    fi
done
echo "  posix9: $SUCCESS compiled, $FAIL failed"
$PPC_AR rcs posix9/libposix9.a posix9/*.o 2>/dev/null || true

echo ""
echo "=== Step 4: Building OS9 platform files ==="
mkdir -p os9
$PPC_CC $CFLAGS $INCLUDES -c "$DROPBEAR_DIR/os9_platform.c" -o os9/os9_platform.o && echo "  os9_platform.c OK" || echo "  os9_platform.c FAIL"
$PPC_CC $CFLAGS $INCLUDES -c "$DROPBEAR_DIR/main_os9.c" -o os9/main_os9.o && echo "  main_os9.c OK" || echo "  main_os9.c FAIL"

echo ""
echo "=== Step 5: Building Dropbear core ==="
mkdir -p dropbear
DBFLAGS="$CFLAGS -DDROPBEAR_SERVER $INCLUDES"
DROPBEAR_SRCS="
dbutil.c buffer.c queue.c
atomicio.c compat.c fake-rfc2553.c
signkey.c rsa.c dss.c ecdsa.c ed25519.c
bignum.c gensignkey.c gendss.c genrsa.c
common-session.c packet.c common-algo.c common-kex.c
common-channel.c common-chansession.c termcodes.c
loginrec.c tcp-accept.c listener.c process-packet.c
dbrandom.c crypto_desc.c curve25519.c
gcm.c chachapoly.c
svr-main.c svr-auth.c svr-authpasswd.c svr-authpubkey.c
svr-session.c svr-service.c svr-chansession.c
svr-kex.c svr-tcpfwd.c svr-agentfwd.c svr-x11fwd.c
"
SUCCESS=0
FAIL=0
for src in $DROPBEAR_SRCS; do
    [ -f "$DROPBEAR_DIR/$src" ] || continue
    obj="dropbear/${src%.c}.o"
    if $PPC_CC $DBFLAGS -c "$DROPBEAR_DIR/$src" -o "$obj" 2>/dev/null; then
        ((SUCCESS++))
    else
        ((FAIL++))
    fi
done
echo "  dropbear core: $SUCCESS compiled, $FAIL failed"

echo ""
echo "=== Libraries created ==="
ls -la libtommath/libtommath.a libtomcrypt/libtomcrypt.a posix9/libposix9.a 2>/dev/null || echo "Some libraries missing"

echo ""
echo "=== Objects created ==="
find . -name "*.o" | wc -l
echo " object files total"

echo ""
echo "Build complete. Check errors above."
