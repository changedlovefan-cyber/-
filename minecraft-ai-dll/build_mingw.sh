#!/usr/bin/env bash
# MinGW-w64 (x86_64-w64-mingw32-g++) で MinecraftAI.dll をビルドする。
#
#   ./build_mingw.sh                    # mingw-w64 が PATH にある場合
#   CXX=x86_64-w64-mingw32-g++ ./build_mingw.sh
#   ./build_mingw.sh --dynamic-runtime   # libstdc++/libgcc を DLL に依存させる
#
# 出力: dist/MinecraftAI.dll (+ dist/config.json)
#
# GDI+ を使わない構成 (JPEG エンコードは WIC) なので、MinGW-w64 の
# gdiplus ヘッダ/インポートライブラリが無い環境でもビルドできる。
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-x86_64-w64-mingw32-g++}"
OUT_DIR="dist"
OUT="$OUT_DIR/MinecraftAI.dll"

if ! command -v "$CXX" >/dev/null 2>&1; then
  echo "エラー: $CXX が見つかりません。" >&2
  echo "  Ubuntu/Debian: sudo apt install g++-mingw-w64-x86-64" >&2
  echo "  MSYS2:         pacman -S mingw-w64-x86_64-gcc" >&2
  exit 1
fi

# DLL なので -static は使えない。libstdc++ / libgcc だけ静的リンクして
# 「Windows 側に libstdc++-6.dll を置かなくても動く」ようにする。
STATIC_FLAGS="-static-libgcc -static-libstdc++"
if [[ "${1:-}" == "--dynamic-runtime" ]]; then
  STATIC_FLAGS=""
fi

mkdir -p "$OUT_DIR"

echo "==> コンパイル: $CXX"
set -x
"$CXX" \
  -shared -O2 -std=c++17 \
  -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DNDEBUG \
  -Wall -Wextra \
  -Isrc \
  -o "$OUT" \
  src/dllmain.cpp \
  src/screen_capture.cpp \
  src/jpeg_encode.cpp \
  src/ollama_client.cpp \
  src/input_sim.cpp \
  $STATIC_FLAGS \
  -lgdi32 -luser32 -lwinhttp -lole32 -loleaut32
set +x

cp -f config.json "$OUT_DIR/config.json"
echo
echo "ビルド完了: $OUT"
ls -la "$OUT"
