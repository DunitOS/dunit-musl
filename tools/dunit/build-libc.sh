#!/bin/sh
# Build the Dunit OS musl port: a static libc.a + crt objects for the
# x86_64-dunit target, installed into a sysroot.
#
# The CPU is still x86_64 and the low-level syscall instruction/register
# convention already matches Dunit (see PORTING.md), so the host x86_64 C
# compiler is the cross compiler here — no separate x86_64-dunit-gcc is needed
# to produce the freestanding, static libc.a. What makes the result a *Dunit*
# libc rather than a Linux one is the syscall-number remap and the Dunit crt/
# adaptation layer carried in this fork, NOT the compiler.
#
# Usage:
#   tools/dunit/build-libc.sh [SYSROOT_DIR]
#
# SYSROOT_DIR defaults to ./build/dunit-sysroot (created if absent). Override
# the toolchain with CC=, AR=, RANLIB= in the environment. The build happens
# out-of-tree so the fork's working tree stays clean.
set -eu

srcdir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
sysroot=${1:-"$(pwd)/build/dunit-sysroot"}
builddir=${DUNIT_MUSL_BUILDDIR:-"$(pwd)/build/dunit-musl-build"}
CC=${CC:-cc}
AR=${AR:-ar}
RANLIB=${RANLIB:-ranlib}

echo "dunit-musl: srcdir=$srcdir"
echo "dunit-musl: sysroot=$sysroot"
echo "dunit-musl: builddir=$builddir  CC=$CC"

rm -rf "$builddir"
mkdir -p "$builddir"
cd "$builddir"

"$srcdir/configure" \
	--target=x86_64-dunit \
	--disable-shared \
	--prefix="$sysroot" \
	CC="$CC" AR="$AR" RANLIB="$RANLIB"

make -j"$(nproc 2>/dev/null || echo 2)"
make install

echo "dunit-musl: built x86_64-dunit libc into $sysroot"
ls -1 "$sysroot/lib/libc.a" "$sysroot/lib/crt1.o" "$sysroot/lib/crti.o" "$sysroot/lib/crtn.o"
