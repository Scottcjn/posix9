#!/bin/bash
# Build ALL Dropbear source files for OS 9

set -e
export PATH=/home/scott/Retro68/build/toolchain/bin:$PATH

BUILD_DIR=build-dropbear-os9
DB_DIR=dropbear-2024.86
DB_SRC=$DB_DIR/src

# Common compile flags
CFLAGS="-O2 \
    -I./include \
    -I./$DB_DIR \
    -I./$DB_SRC \
    -I./libtomcrypt-1.18.2/src/headers \
    -I./libtommath-1.3.0 \
    -include ./$DB_DIR/os9_config.h \
    -DLTC_NO_FILE \
    -DLTM_DESC \
    -DUSE_LTM \
    -DDROPBEAR_SERVER=1 \
    -DDROPBEAR_MULTI=1 \
    -DDBMULTI_dropbear=1"

# Compile function
compile() {
    local src=$1
    local name=$(basename "$src" .c)
    local obj=$BUILD_DIR/dropbear/${name}.o

    if [ -f "$obj" ] && [ "$obj" -nt "$src" ]; then
        echo "  Skip: $name.o (up to date)"
        return 0
    fi

    echo "  Compile: $name.c"
    powerpc-apple-macos-gcc -c $CFLAGS "$src" -o "$obj" 2>&1 || {
        echo "  FAILED: $name.c"
        return 1
    }
}

mkdir -p $BUILD_DIR/dropbear

echo "Building Dropbear source files..."

# Server sources (svr-*)
for src in $DB_SRC/svr-*.c; do
    compile "$src" || true
done

# Common sources
for src in $DB_SRC/common-*.c; do
    compile "$src" || true
done

# Core sources
for src in atomicio buffer packet queue; do
    compile "$DB_SRC/${src}.c" || true
done

# Crypto sources
for src in bignum crypto_desc dbrandom; do
    compile "$DB_SRC/${src}.c" || true
done

# Key sources
for src in signkey gensignkey genrsa rsa; do
    compile "$DB_SRC/${src}.c" || true
done

# Other sources we need
for src in compat dbmalloc dbutil fake-rfc2553 listener netio process-packet tcp-accept termcodes; do
    compile "$DB_SRC/${src}.c" || true
done

# Additional sources that might be needed
for src in runopts channel circbuffer curve25519 dss ecc ed25519 gcm_mode gendss gened25519 loginrec pty sftpserver x11fwd; do
    if [ -f "$DB_SRC/${src}.c" ]; then
        compile "$DB_SRC/${src}.c" || true
    fi
done

# Count results
total=$(ls -1 $BUILD_DIR/dropbear/*.o 2>/dev/null | wc -l)
echo ""
echo "Compiled $total object files"
echo ""

# Create library
echo "Creating libdropbear.a..."
powerpc-apple-macos-ar rcs $BUILD_DIR/libdropbear.a $BUILD_DIR/dropbear/*.o
echo "Done!"
