#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
CC=${CC:-"$STOVE/prefix/bin/m68k-amigaos-gcc"}
AR=${AR:-"$STOVE/prefix/bin/m68k-amigaos-ar"}
OUT=${OUT:-"$HERE/build-amiga"}

mkdir -p "$OUT"
"$CC" -m68040 -m68881 -O2 -fno-delete-null-pointer-checks -std=c99 -Wall -Wextra -Werror -pedantic \
  -I"$HERE/include" -c "$HERE/src/openlayout.c" -o "$OUT/openlayout.o"
"$AR" rcs "$OUT/libopenlayout.a" "$OUT/openlayout.o"
"$CC" -m68040 -m68881 -O2 -fno-delete-null-pointer-checks -std=c99 -Wall -Wextra -Werror -pedantic -noixemul \
  -I"$HERE/include" "$HERE/tests/test_layout.c" "$OUT/libopenlayout.a" \
  -o "$OUT/LayoutTest"
echo "$OUT/libopenlayout.a ($(wc -c < "$OUT/libopenlayout.a") bytes)"
echo "$OUT/LayoutTest ($(wc -c < "$OUT/LayoutTest") bytes)"
sha256sum "$OUT/libopenlayout.a" "$OUT/LayoutTest"
