#!/usr/bin/env bash
#
# mayhem/build.sh — build Catime's fuzz harnesses and the functional checks.
#
# Catime is a Windows-only Win32 app (MinGW/CMake, GDI+/WIC). There is no Linux build, so this
# compiles only the portable parser sources that read untrusted data natively with clang, against
# a small harness-owned Win32 shim (mayhem/shim/windows.h + win32_shim.c: types, a POSIX-backed
# CreateFileW/ReadFile/SetFilePointerEx/GetFileSizeEx and UTF-8<->UTF-16 conversion).
# Everything lands in build-mayhem/ (gitignored upstream via `build-*/`), so the tree survives
# `git clean -ffdX` and a re-run rebuilds in place.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
OUT="$SRC/build-mayhem"
mkdir -p "$OUT/obj" "$OUT/test-obj"
INC=(-I"$SRC/mayhem/shim" -I"$SRC/include" -I"$SRC/src" -I"$SRC/libs/stb")
CFLAGS_FUZZ="$SANITIZER_FLAGS $DEBUG_FLAGS -std=gnu11"

# compile <objdir> <flags> <src...> -> prints the object paths (one per source)
compile() {
  local dir="$1" flags="$2"; shift 2
  local s o
  for s in "$@"; do
    o="$dir/$(echo "${s#$SRC/}" | tr '/' '_').o"
    # shellcheck disable=SC2086
    $CC $flags "${INC[@]}" -c "$s" -o "$o" >&2
    echo "$o"
  done
}

# shellcheck disable=SC2086
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OUT/obj/lsan_off.o"
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/obj/standalone_main.o"

# fuzz_target <name> <extra cflags> <project sources...>
# The fuzzer objects carry SanCov (-fsanitize=fuzzer-no-link); the standalone reproducer is built
# from separate objects without it, so it also links with an empty SANITIZER_FLAGS.
fuzz_target() {
  local name="$1" extra="$2"; shift 2
  local d="$OUT/obj/$name"; mkdir -p "$d/fuzz" "$d/standalone"
  mapfile -t fobjs < <(compile "$d/fuzz" "$CFLAGS_FUZZ -fsanitize=fuzzer-no-link $extra" "$SRC/mayhem/harnesses/$name.c" "$@")
  mapfile -t sobjs < <(compile "$d/standalone" "$CFLAGS_FUZZ $extra" "$SRC/mayhem/harnesses/$name.c" "$@")
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "${fobjs[@]}" "$OUT/obj/lsan_off.o" -lm -o "$OUT/$name"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "${sobjs[@]}" "$OUT/obj/standalone_main.o" "$OUT/obj/lsan_off.o" -lm -o "$OUT/$name-standalone"
}

SHIM="$SRC/mayhem/shim/win32_shim.c"
# stb_truetype is header-only; the harness TU defines STB_TRUETYPE_IMPLEMENTATION like
# src/drawing/drawing_text_stb_stb.c does.
fuzz_target fuzz_stb_truetype ""
# WCHAR must be 16-bit like on Windows: the name-table decoder casts UTF-16BE bytes to WCHAR*.
fuzz_target fuzz_font_name "-fshort-wchar" \
  "$SRC/src/font/font_ttf_parser.c" "$SRC/src/font/font_ttf_file.c" "$SRC/src/utils/string_convert.c" "$SHIM"
fuzz_target fuzz_time_parser "" \
  "$SRC/src/utils/time_parser.c" "$SRC/src/utils/time_parser_advanced.c" "$SRC/src/utils/time_format.c"
fuzz_target fuzz_color_parser "" \
  "$SRC/src/color/color_parser.c" "$SRC/src/color/gradient.c" "$SRC/src/color/color_conversion.c" \
  "$SRC/src/utils/string_safe.c"

CRT="$SRC/mayhem/shim/crt_shim.c"
# Markdown parser: every portable parser TU; the GDI renderer, image download/render and the
# checkbox file-rewriter (markdown_checkbox.c) are Win32 UI code and stay out.
fuzz_target fuzz_markdown "-I$SRC/src/markdown" \
  "$SRC"/src/markdown/markdown_{block,inline,inline_count,inline_link,inline_style,inline_tag_count,inline_tags,parser,parser_rich,state}.c \
  "$SRC/src/utils/string_convert.c" "$SRC/src/utils/url_safety.c" "$SHIM" "$CRT"
fuzz_target fuzz_update_json "" \
  "$SRC/src/update/update_json.c" "$SRC/src/update/update_parser.c" "$SRC/src/utils/url_safety.c" "$SHIM" "$CRT"
# .ani decoder; icon creation goes through mayhem/shim/gdi_shim.c (no user32 off Windows).
fuzz_target fuzz_ani "-I$SRC/src/tray" \
  "$SRC/src/tray/tray_animation_decoder_ani.c" "$SRC/src/tray/tray_animation_decoder_ani_sequence.c" \
  "$SHIM" "$SRC/mayhem/shim/gdi_shim.c"

# Functional checks with NORMAL flags (no sanitizers): upstream's own time parser unit test, and
# a known-answer font-name check over the fonts upstream ships in asset/font.
TF="-O1 $COVERAGE_FLAGS"
mapfile -t tobjs < <(compile "$OUT/test-obj" "$TF" "$SRC/tests/time_parser_tests.c" \
  "$SRC/src/utils/time_parser.c" "$SRC/src/utils/time_parser_advanced.c" "$SRC/src/utils/time_format.c")
# shellcheck disable=SC2086
$CC $COVERAGE_FLAGS "${tobjs[@]}" -o "$OUT/time_parser_tests"
mkdir -p "$OUT/test-obj/font"
mapfile -t fobjs < <(compile "$OUT/test-obj/font" "$TF -fshort-wchar" "$SRC/mayhem/tests/font_name_check.c" \
  "$SRC/src/font/font_ttf_parser.c" "$SRC/src/font/font_ttf_file.c" "$SRC/src/utils/string_convert.c" "$SHIM")
# shellcheck disable=SC2086
$CC $COVERAGE_FLAGS "${fobjs[@]}" -o "$OUT/font_name_check"
mkdir -p "$OUT/test-obj/ani"
mapfile -t aobjs < <(compile "$OUT/test-obj/ani" "$TF -I$SRC/src/tray -DANI_CHECK_NO_FUZZ_ENTRY" \
  "$SRC/mayhem/tests/ani_check.c" "$SRC/mayhem/harnesses/fuzz_ani.c" \
  "$SRC/src/tray/tray_animation_decoder_ani.c" "$SRC/src/tray/tray_animation_decoder_ani_sequence.c" \
  "$SHIM" "$SRC/mayhem/shim/gdi_shim.c")
# shellcheck disable=SC2086
$CC $COVERAGE_FLAGS "${aobjs[@]}" -o "$OUT/ani_check"
echo "build.sh: done"
