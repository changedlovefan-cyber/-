#include "jpeg_encode.h"
#include "base64.h"
#include <vector>
#include <algorithm>
#include <cwchar>

// =============================================================================
// GDI+ 版 (MSVC 用)
// =============================================================================
#ifdef DX_WITH_GDI_PLUS

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

static int GetEncoderClsid(const WCHAR* format, CLSID* clsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    std::vector<unsigned char> buf(size);
    Gdiplus::ImageCodecInfo* info = (Gdiplus::ImageCodecInfo*)buf.data();
    Gdiplus::GetImageEncoders(num, size, info);
    for (UINT i = 0; i < num; i++) {
        if (wcscmp(info[i].MimeType, format) == 0) { *clsid = info[i].Clsid; return (int)i; }
    }
    return -1;
}

std::string EncodePixelsToBase64Jpeg(const unsigned char* pixels, int width, int height,
                                     int targetWidth) {
    if (!pixels || width <= 0 || height <= 0) return "";
    if (targetWidth <= 0) targetWidth = 800;
    int th = (int)((double)height * targetWidth / width);
    if (th <= 0) th = 450;

    // HBITMAP 経由で GDI+ に渡す
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(nullptr);
    HBITMAP hBmp = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, nullptr, nullptr, 0);
    if (!hBmp) { ReleaseDC(nullptr, hdc); return ""; }
    SetDIBits(hdc, hBmp, 0, (UINT)height, pixels, &bi, DIB_RGB_COLORS);
    ReleaseDC(nullptr, hdc);

    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromHBITMAP(hBmp, nullptr);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        if (bmp) delete bmp;
        DeleteObject(hBmp);
        return "";
    }
    Gdiplus::Bitmap dst(targetWidth, th, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&dst);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.DrawImage(bmp, 0, 0, targetWidth, th);
    }
    delete bmp;
    DeleteObject(hBmp);

    CLSID jpgClsid;
    if (GetEncoderClsid(L"image/jpeg", &jpgClsid) < 0) return "";

    Gdiplus::EncoderParameters encParams;
    encParams.Count = 1;
    encParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
    encParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
    encParams.Parameter[0].NumberOfValues = 1;
    ULONG quality = 80;
    encParams.Parameter[0].Value = &quality;

    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, 0);
    IStream* stream = nullptr;
    if (!hGlobal || FAILED(CreateStreamOnHGlobal(hGlobal, TRUE, &stream))) {
        if (hGlobal) GlobalFree(hGlobal);
        return "";
    }
    std::vector<unsigned char> jpeg;
    if (dst.Save(stream, &jpgClsid, &encParams) == Gdiplus::Ok) {
        LARGE_INTEGER zero = {};
        ULARGE_INTEGER sz = {};
        if (SUCCEEDED(stream->Seek(zero, STREAM_SEEK_END, &sz)) && sz.QuadPart > 0) {
            stream->Seek(zero, STREAM_SEEK_SET, nullptr);
            jpeg.resize((size_t)sz.QuadPart);
            ULONG got = 0;
            stream->Read(jpeg.data(), (ULONG)jpeg.size(), &got);
            jpeg.resize(got);
        }
    }
    stream->Release();
    GlobalFree(hGlobal);
    if (jpeg.empty()) return "";
    return Base64Encode(jpeg);
}

bool SelfTestJpegRoundTrip(std::string& message) {
    // 実際の経路（縮小 + JPEG + base64）を 1 回通して確かめる
    std::vector<unsigned char> px(64 * 64 * 4);
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            size_t i = ((size_t)y * 64 + x) * 4;
            px[i + 0] = (unsigned char)(x * 4);  // B
            px[i + 1] = (unsigned char)(y * 4);  // G
            px[i + 2] = 128;                     // R
            px[i + 3] = 255;                     // A
        }
    }
    std::string b64 = EncodePixelsToBase64Jpeg(px.data(), 64, 64, 64);
    if (b64.size() < 100 || b64.compare(0, 4, "/9j/") != 0) {
        message = "GDI+ JPEG エンコード失敗";
        return false;
    }
    message = "GDI+ JPEG " + std::to_string(b64.size()) + " bytes (base64)";
    return true;
}

// =============================================================================
// WIC 版 (MinGW / GDI+ なし環境用)
// =============================================================================
#else

#include <wincodec.h>
#include <objbase.h>

// ---- WIC の GUID を自前定義 ----
// wincodec.h の GUID_* は外部シンボル (uuid ライブラリ or DEFINE_GUID の実体) を
// 参照するため、MSVC では「未解決の外部シンボル」になりやすい。
// 値は WIC のピクセルフォーマット仕様（共通プレフィックス 6fddc324-...-c9 + 連番）。
static const CLSID kCLSID_WICImagingFactory = {
    0xCACAF262, 0x9370, 0x4615, {0xA1, 0x3B, 0x9F, 0x55, 0x39, 0xDA, 0x4C, 0x0A}};
static const IID kIID_IWICImagingFactory = {
    0xEC5EC8A9, 0xC395, 0x4314, {0x9C, 0x77, 0x54, 0xD7, 0xA9, 0x35, 0xFF, 0x70}};
static const GUID kWICPixelFormat32bppBGRA = {
    0x6FDDC324, 0x4E03, 0x4BFE, {0xB1, 0x85, 0x3D, 0x77, 0x76, 0x8D, 0xC9, 0x0F}};
static const GUID kWICPixelFormat24bppBGR = {
    0x6FDDC324, 0x4E03, 0x4BFE, {0xB1, 0x85, 0x3D, 0x77, 0x76, 0x8D, 0xC9, 0x0C}};
static const GUID kWICContainerFormatJpeg = {
    0x19E4A5AA, 0x5662, 0x4FC5, {0xA0, 0xC0, 0x17, 0x58, 0x02, 0x8E, 0x10, 0x57}};

// windowscodecs.dll を直接ロードする（-lwindowscodecs 不要）。
static HMODULE LoadWic() {
    HMODULE h = LoadLibraryW(L"windowscodecs.dll");
    return h;
}

bool SelfTestJpegRoundTrip(std::string& message) {
    // 実際の経路（WIC 初期化 → 縮小 → JPEG → base64）を 1 回通して確かめる。
    std::vector<unsigned char> px(64 * 64 * 4);
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            size_t i = ((size_t)y * 64 + x) * 4;
            px[i + 0] = (unsigned char)(x * 4);  // B
            px[i + 1] = (unsigned char)(y * 4);  // G
            px[i + 2] = 128;                     // R
            px[i + 3] = 255;                     // A
        }
    }
    std::string b64 = EncodePixelsToBase64Jpeg(px.data(), 64, 64, 64);
    if (b64.size() < 100 || b64.compare(0, 4, "/9j/") != 0) {
        message = "WIC JPEG エンコード失敗 (windowscodecs.dll / CoInitialize を確認)";
        return false;
    }
    message = "WIC JPEG " + std::to_string(b64.size()) + " bytes (base64)";
    return true;
}

std::string EncodePixelsToBase64Jpeg(const unsigned char* pixels, int width, int height,
                                     int targetWidth) {
    if (!pixels || width <= 0 || height <= 0) return "";
    if (targetWidth <= 0) targetWidth = 800;
    int th = (int)((double)height * targetWidth / width);
    if (th <= 0) th = 450;

    HMODULE hMod = LoadWic();
    if (!hMod) return "";

    // WIC のプロキシは内部で COM を使うので、このスレッドで COM を初期化しておく。
    // ゲームが既に別モードで初期化済みでも RPC_E_CHANGED_MODE で済むので問題ない。
    struct ComScope {
        HRESULT hr;
        ComScope() { hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
        ~ComScope() { if (SUCCEEDED(hr)) CoUninitialize(); }
    } comScope;

    // ---- 動的ロード (インポートライブラリ不要) ----
    typedef HRESULT(WINAPI* PFN_CreateStream)(HGLOBAL, BOOL, IStream**);
    PFN_CreateStream pCreateStream =
        (PFN_CreateStream)GetProcAddress(GetModuleHandleW(L"ole32.dll"), "CreateStreamOnHGlobal");
    if (!pCreateStream) { FreeLibrary(hMod); return ""; }

    // WIC の ImagingFactory はプロキシ経由 (DLL 内のエクスポート) で作る
    typedef HRESULT(WINAPI* PFN_WICCreateFactory)(REFCLSID, REFIID, void**);
    PFN_WICCreateFactory pWicFactory =
        (PFN_WICCreateFactory)GetProcAddress(hMod, "WICCreateImagingFactory_Proxy");
    if (!pWicFactory) { FreeLibrary(hMod); return ""; }

    IWICImagingFactory* pFactory = nullptr;
    if (FAILED(pWicFactory(kCLSID_WICImagingFactory, kIID_IWICImagingFactory, (void**)&pFactory)) || !pFactory) {
        FreeLibrary(hMod);
        return "";
    }

    std::string result;
    IWICBitmap* pSrcBmp = nullptr;
    IWICBitmapScaler* pScaler = nullptr;
    IWICFormatConverter* pConv = nullptr;
    IWICBitmapEncoder* pEncoder = nullptr;
    IWICBitmapFrameEncode* pFrame = nullptr;
    IPropertyBag2* pProps = nullptr;
    IStream* pStream = nullptr;
    HGLOBAL hGlobal = nullptr;

    do {
        // 元画像 (32bppBGRA, トップダウン)
        // WriteSource の矩形は「ソース(縮小後)の座標系」で指定する
        WICRect rcOut = {0, 0, targetWidth, th};
        if (FAILED(pFactory->CreateBitmapFromMemory(
                (UINT)width, (UINT)height, kWICPixelFormat32bppBGRA,
                (UINT)(width * 4), (UINT)(width * height * 4), (BYTE*)pixels, &pSrcBmp)))
            break;

        // 縮小
        if (FAILED(pFactory->CreateBitmapScaler(&pScaler))) break;
        if (FAILED(pScaler->Initialize(pSrcBmp, (UINT)targetWidth, (UINT)th, WICBitmapInterpolationModeFant)))
            break;

        // JPEG は 24bppBGR でエンコードする
        if (FAILED(pFactory->CreateFormatConverter(&pConv))) break;
        if (FAILED(pConv->Initialize(pScaler, kWICPixelFormat24bppBGR,
                                     WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom)))
            break;

        // 出力ストリーム
        hGlobal = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (!hGlobal) break;
        if (FAILED(pCreateStream(hGlobal, FALSE, &pStream)) || !pStream) break;

        if (FAILED(pFactory->CreateEncoder(kWICContainerFormatJpeg, nullptr, &pEncoder))) break;
        if (FAILED(pEncoder->Initialize(pStream, WICBitmapEncoderNoCache))) break;
        if (FAILED(pEncoder->CreateNewFrame(&pFrame, &pProps))) break;

        // 品質 80
        if (pProps) {
            PROPBAG2 opt = {};
            opt.pstrName = (LPOLESTR)L"ImageQuality";
            VARIANT v;
            VariantInit(&v);
            v.vt = VT_R4;
            v.fltVal = 0.8f;
            if (SUCCEEDED(pProps->Write(1, &opt, &v))) {
                // 書き込めない環境でも致命ではない
            }
            VariantClear(&v);
        }
        if (FAILED(pFrame->Initialize(pProps))) break;
        if (FAILED(pFrame->SetSize((UINT)targetWidth, (UINT)th))) break;

        WICPixelFormatGUID fmt = kWICPixelFormat24bppBGR;
        if (FAILED(pFrame->SetPixelFormat(&fmt))) break;
        if (FAILED(pFrame->WriteSource(pConv, &rcOut))) break;
        if (FAILED(pFrame->Commit())) break;
        if (FAILED(pEncoder->Commit())) break;

        // ストリームから取り出し
        STATSTG stat = {};
        if (FAILED(pStream->Stat(&stat, STATFLAG_NONAME))) break;
        ULONG size = (ULONG)stat.cbSize.QuadPart;
        if (size == 0) break;
        LARGE_INTEGER zero = {};
        pStream->Seek(zero, STREAM_SEEK_SET, nullptr);
        std::vector<unsigned char> jpeg(size);
        ULONG got = 0;
        if (FAILED(pStream->Read(jpeg.data(), size, &got)) || got == 0) break;
        jpeg.resize(got);
        result = Base64Encode(jpeg);
    } while (false);

    if (pProps) pProps->Release();
    if (pFrame) pFrame->Release();
    if (pEncoder) pEncoder->Release();
    if (pConv) pConv->Release();
    if (pScaler) pScaler->Release();
    if (pSrcBmp) pSrcBmp->Release();
    if (pStream) pStream->Release();
    if (hGlobal) GlobalFree(hGlobal);
    pFactory->Release();
    FreeLibrary(hMod);
    return result;
}

#endif // DX_WITH_GDI_PLUS
