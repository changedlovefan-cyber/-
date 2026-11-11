#pragma once
#include <windows.h>
#include <string>
#include <vector>

// =============================================================================
// 画像 → JPEG(base64) 変換。2 つの実装を切り替え可能。
//
//   * DX_WITH_GDI_PLUS (MSVC / フルの MinGW-w64)
//       GDI+ (gdiplus.dll) で縮小 + JPEG エンコード。従来の実装。
//   * それ以外 (zig 同梱 MinGW / GDI+ の無い環境)
//       WIC (windowscodecs.dll) で縮小 + JPEG エンコード。Win7 以降の標準。
//
// どちらも同じシグネチャ・同じ挙動（縮小して品質 80 の JPEG → base64）。
// マクロはビルド側 (CMakeLists.txt / build_mingw.sh) が定義する。
// =============================================================================

// 32bpp ARGB のピクセル列 (トップダウン) を、幅 targetWidth に縮小して JPEG 化し base64 で返す。
// 失敗時は空文字列。pixels は width*height*4 バイト必要。
std::string EncodePixelsToBase64Jpeg(const unsigned char* pixels, int width, int height,
                                     int targetWidth);

// テスト用: 埋め込み JPEG (src/jpeg_embed.h) をデコード → 再エンコードして
// 経路が動くか確認する。成功なら true。
bool SelfTestJpegRoundTrip(std::string& message);
