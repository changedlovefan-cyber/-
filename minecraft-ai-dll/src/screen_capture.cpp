#include "screen_capture.h"
#include "jpeg_encode.h"
#include <vector>
#include <algorithm>

void ResolveDllPaths(HMODULE hModule, std::wstring& dllDir, std::wstring& logPath) {
    WCHAR path[MAX_PATH] = {};
    GetModuleFileNameW(hModule, path, MAX_PATH);
    std::wstring full(path);
    std::wstring::size_type pos = full.find_last_of(L"\\/");
    dllDir = (pos == std::wstring::npos) ? std::wstring(L".") : full.substr(0, pos);
    logPath = dllDir + L"\\minecraft_ai.log";
}

HWND FindMinecraftWindow() {
    // 1) クラス名で探す (Bedrock は "Minecraft" 系が多い)
    HWND h = FindWindowW(L"Minecraft", nullptr);
    if (h) return h;
    // 2) タイトルで探す
    h = FindWindowW(nullptr, L"Minecraft");
    if (h) return h;
    // 3) EnumWindows でタイトルに Minecraft を含むものを探す
    struct Ctx { HWND found; };
    Ctx ctx{nullptr};
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        Ctx* c = (Ctx*)lParam;
        if (!IsWindowVisible(hwnd)) return TRUE;
        if (IsIconic(hwnd)) return TRUE; // 最小化中はキャプチャしても真っ黒
        WCHAR title[256] = {};
        GetWindowTextW(hwnd, title, 256);
        if (wcsstr(title, L"Minecraft")) { c->found = hwnd; return FALSE; }
        return TRUE;
    }, (LPARAM)&ctx);
    return ctx.found;
}

// 32bpp トップダウン DIB を読んで、ほぼ真っ黒かどうか判定する。
// PrintWindow が GPU/DirectComposition 面で「成功したのに真っ黒」を返すケースの検出用。
static bool IsMostlyBlack(const std::vector<unsigned char>& pixels, int w, int h) {
    if (pixels.empty()) return false;
    long long sum = 0;
    long long count = 0;
    // 16 ピクセルごとにサンプル
    for (int y = 0; y < h; y += 16) {
        for (int x = 0; x < w; x += 16) {
            size_t idx = ((size_t)y * w + x) * 4;
            if (idx + 2 >= pixels.size()) continue;
            int lum = (pixels[idx] + pixels[idx + 1] + pixels[idx + 2]) / 3;
            sum += lum;
            count++;
        }
    }
    if (count == 0) return false;
    double avg = (double)sum / count;
    return avg < 4.0; // 平均輝度がほぼ 0 なら真っ黒
}

// 画面上のウィンドウ矩形を BitBlt で読む（32bpp トップダウン）
static bool CaptureScreenRegion(const RECT& rc, int w, int h, std::vector<unsigned char>& out) {
    HDC hdcScreen = GetDC(nullptr);
    if (!hdcScreen) return false;
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, w, h);
    if (!hdcMem || !hBmp) {
        if (hBmp) DeleteObject(hBmp);
        if (hdcMem) DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);
        return false;
    }
    HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBmp);
    BOOL ok = BitBlt(hdcMem, 0, 0, w, h, hdcScreen, rc.left, rc.top, SRCCOPY);

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    out.resize((size_t)w * h * 4);
    bool got = ok && GetDIBits(hdcMem, hBmp, 0, (UINT)h, out.data(), &bi, DIB_RGB_COLORS) != 0;
    if (!got) out.clear();

    SelectObject(hdcMem, hOld);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    return got;
}

std::string CaptureWindowToBase64(HWND hwnd, int targetWidth) {
    if (!hwnd || !IsWindow(hwnd)) return "";
    if (IsIconic(hwnd)) return ""; // 最小化中
    RECT rc = {};
    if (!GetWindowRect(hwnd, &rc)) return "";
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return "";
    if (targetWidth <= 0) targetWidth = 800;

    std::vector<unsigned char> pixels;

    // --- 経路 1: PrintWindow + PW_RENDERFULLCONTENT ---
    // UWP / DWM で黒画面を避ける基本。ただし GPU 面では黒くなるので検出して次へ。
    {
        HDC hdcScreen = GetDC(nullptr);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, w, h);
        if (hdcMem && hBmp) {
            HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBmp);
            BOOL ok = PrintWindow(hwnd, hdcMem, PW_RENDERFULLCONTENT);
            if (ok) {
                BITMAPINFO bi = {};
                bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bi.bmiHeader.biWidth = w;
                bi.bmiHeader.biHeight = -h;
                bi.bmiHeader.biPlanes = 1;
                bi.bmiHeader.biBitCount = 32;
                bi.bmiHeader.biCompression = BI_RGB;
                pixels.resize((size_t)w * h * 4);
                if (!GetDIBits(hdcMem, hBmp, 0, (UINT)h, pixels.data(), &bi, DIB_RGB_COLORS)) {
                    pixels.clear();
                }
            }
            SelectObject(hdcMem, hOld);
        }
        if (hBmp) DeleteObject(hBmp);
        if (hdcMem) DeleteDC(hdcMem);
        if (hdcScreen) ReleaseDC(nullptr, hdcScreen);
    }

    // --- 経路 2: 黒フレームなら画面上の矩形を BitBlt ---
    if (pixels.empty() || IsMostlyBlack(pixels, w, h)) {
        std::vector<unsigned char> alt;
        if (CaptureScreenRegion(rc, w, h, alt)) {
            if (pixels.empty() || !IsMostlyBlack(alt, w, h)) {
                pixels.swap(alt);
            }
        }
    }
    if (pixels.empty()) return "";

    return EncodePixelsToBase64Jpeg(pixels.data(), w, h, targetWidth);
}
