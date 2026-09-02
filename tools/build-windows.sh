#!/usr/bin/env bash
# Cross-kompilacja wersji dla Windows z poziomu Linuksa.
#
# Dlaczego akurat tak: oficjalne binarki SFML dla MinGW sa budowane pod UCRT, a mingw-w64
# z repozytoriow Debiana/Ubuntu celuje w stary MSVCRT. Roznia sie przez to typem mbstate_t,
# co objawia sie bledami linkera w rodzaju "undefined reference to
# std::__codecvt_utf8_utf16_base<wchar_t>::do_out(_Mbstatet&, ...)". Dlatego bierzemy SFML
# z repozytorium MSYS2 dla srodowiska MINGW64, ktore rowniez jest oparte na MSVCRT -
# wtedy ABI sie zgadza i wszystko linkuje sie statycznie.
#
# Wymagania: g++-mingw-w64-x86-64 (wariant posix), curl, zstd, tar.
# Uzycie: ./tools/build-windows.sh   (albo: make windows)

set -euo pipefail

RPS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS_DIR="$RPS_DIR/build/win-deps"
PREFIX="$DEPS_DIR/mingw64"
OUT_DIR="$RPS_DIR/precompiled/windows"
OBJ_DIR="$RPS_DIR/build/win-obj"

CXX=x86_64-w64-mingw32-g++-posix
command -v "$CXX" >/dev/null || {
    echo "Brak $CXX - zainstaluj: sudo apt install g++-mingw-w64-x86-64" >&2
    exit 1
}

# --- 1. Zaleznosci z MSYS2 (pobierane raz, potem cache w build/win-deps) ---
MSYS_REPO="https://repo.msys2.org/mingw/mingw64"
PACKAGES=(sfml freetype zlib bzip2 libpng brotli harfbuzz graphite2)

if [ ! -d "$PREFIX/lib" ]; then
    echo "Pobieram zaleznosci Windows z MSYS2 (jednorazowo)..."
    mkdir -p "$DEPS_DIR"
    INDEX="$DEPS_DIR/index.html"
    curl -sL "$MSYS_REPO/" -o "$INDEX"
    for p in "${PACKAGES[@]}"; do
        file=$(grep -oE "mingw-w64-x86_64-$p-[0-9][^\"]*\.pkg\.tar\.zst" "$INDEX" | sort -V | tail -1)
        [ -n "$file" ] || { echo "Nie znalazlem pakietu: $p" >&2; exit 1; }
        echo "  $file"
        curl -sL "$MSYS_REPO/$file" -o "$DEPS_DIR/$file"
        tar --use-compress-program=unzstd -xf "$DEPS_DIR/$file" -C "$DEPS_DIR" 2>/dev/null || true
    done
fi

# --- 2. Kompilacja ---
echo "Kompiluje dla Windows..."
rm -rf "$OBJ_DIR"; mkdir -p "$OBJ_DIR" "$OUT_DIR"
cd "$RPS_DIR"

FLAGS=(-std=c++17 -O2 -DSFML_STATIC
       -Ithird_party/imgui -Ithird_party/imgui-sfml -Isrc
       -I"$PREFIX/include")

for f in $(find src third_party -name '*.cpp'); do
    "$CXX" "${FLAGS[@]}" -c "$f" -o "$OBJ_DIR/$(echo "$f" | tr '/' '_').o"
done

# --- 3. Linkowanie (statyczne - jeden plik .exe, zero DLL-i do dorzucania) ---
echo "Linkuje..."
"$CXX" "$OBJ_DIR"/*.o -o "$OUT_DIR/rps.exe" \
    -L"$PREFIX/lib" \
    -lsfml-graphics-s -lsfml-window-s -lsfml-system-s \
    -lfreetype -lharfbuzz -lgraphite2 -lpng16 -lbrotlidec -lbrotlicommon -lbz2 -lz \
    -lopengl32 -lwinmm -lgdi32 -lole32 -luuid -lrpcrt4 -lusp10 -ldwrite \
    -static -static-libgcc -static-libstdc++ \
    -mwindows

# --- 4. Dorzucenie wytrenowanego ziarna, zeby nowy profil nie zaczynal od zera ---
if [ -f "$RPS_DIR/data/seed.txt" ]; then
    mkdir -p "$OUT_DIR/data"
    cp "$RPS_DIR/data/seed.txt" "$OUT_DIR/data/"
fi

echo
echo "Gotowe: $OUT_DIR/rps.exe"
x86_64-w64-mingw32-objdump -p "$OUT_DIR/rps.exe" | grep "DLL Name:" | sort -u | sed 's/^/  zalezny od: /'
