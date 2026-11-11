// Minecraft 統合版 AI 自動操作 DLL
// FateInjector で Minecraft.Windows.exe に inject すると、
// 画面キャプチャ → Ollama (gemma4) → SendInput のループが始まる。
//
// 操作:
//   F8 : AI ON/OFF トグル
//   F9 : 緊急停止（全キー解除 + ループ終了）
// ログ: DLL と同じフォルダの minecraft_ai.log
//
// ビルド: MSVC (build.bat) でも MinGW-w64 (build_mingw.sh) でも可。
//   GDI+ を使わない構成にしてあるので MinGW でもそのままリンクできる。
#include <windows.h>
#include <string>
#include "config.h"
#include "screen_capture.h"
#include "jpeg_encode.h"
#include "ollama_client.h"
#include "input_sim.h"

static HMODULE g_hModule = nullptr;
static HANDLE g_thread = nullptr;
static volatile bool g_running = false;
static volatile bool g_aiEnabled = true;
static std::wstring g_dllDir;
static std::wstring g_logPath;

static void Log(const std::string& msg) {
    OutputDebugStringA(("[MinecraftAI] " + msg + "\n").c_str());
    HANDLE h = CreateFileW(g_logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        std::string line = msg + "\r\n";
        DWORD w = 0;
        WriteFile(h, line.c_str(), (DWORD)line.size(), &w, nullptr);
        CloseHandle(h);
    }
}

static DWORD WINAPI WorkerThread(LPVOID) {
    Config cfg = LoadConfig(g_dllDir + L"\\MinecraftAI.dll");
    if (cfg.model.empty()) cfg.model = "gemma4:e4b";
    g_aiEnabled = cfg.enabledByDefault;

    Log("=== MinecraftAI.dll injected ===");
    Log("Worker start. model=" + cfg.model + " goal=" + cfg.goal);

    // JPEG エンコード経路の自己診断（WIC / GDI+ が動くか）
    {
        std::string stMsg;
        if (SelfTestJpegRoundTrip(stMsg)) {
            Log("JPEG encoder OK: " + stMsg);
        } else {
            Log("JPEG encoder NG: " + stMsg + " (キャプチャは失敗します)");
        }
    }
    Log("F8 = AI ON/OFF, F9 = emergency stop");

    bool f8Prev = false, f9Prev = false;
    int captureFailCount = 0;
    while (g_running) {
        // F8 トグル
        bool f8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
        if (f8 && !f8Prev) {
            g_aiEnabled = !g_aiEnabled;
            Log(g_aiEnabled ? "AI ON (F8)" : "AI OFF (F8)");
            if (!g_aiEnabled) ReleaseAllKeys();
        }
        f8Prev = f8;

        // F9 緊急停止
        bool f9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        if (f9 && !f9Prev) {
            Log("Emergency stop (F9)");
            ReleaseAllKeys();
            g_running = false;
            break;
        }
        f9Prev = f9;

        if (!g_aiEnabled) { Sleep(200); continue; }

        HWND hwnd = FindMinecraftWindow();
        if (!hwnd) {
            Log("Minecraft window not found. retry in 2s");
            Sleep(2000);
            continue;
        }

        std::string img = CaptureWindowToBase64(hwnd, cfg.captureWidth);
        if (img.empty()) {
            captureFailCount++;
            if (captureFailCount == 1) {
                Log("Capture failed. ウィンドウが最小化されていないか、前面にあるか確認してください。");
            }
            Sleep(1000);
            continue;
        }
        captureFailCount = 0;

        AIAction act;
        std::string raw;
        if (!AskGemma(img, cfg, act, raw)) {
            Log("Ollama request failed. Ollama running? model=" + cfg.model + " (raw head: " +
                raw.substr(0, 200) + ")");
            Sleep(2000);
            continue;
        }

        std::string keys;
        for (size_t i = 0; i < act.keys.size(); i++) {
            keys += act.keys[i];
            keys += " ";
        }
        Log("AI -> keys:[" + keys + "] dx=" + std::to_string(act.mouseDx) +
            " dy=" + std::to_string(act.mouseDy) +
            " L=" + (act.clickLeft ? "1" : "0") +
            " R=" + (act.clickRight ? "1" : "0") +
            " dur=" + std::to_string(act.durationMs) + " reason=" + act.reason);

        ExecuteAction(act);
        Sleep(cfg.intervalMs);
    }

    ReleaseAllKeys();
    Log("Worker exit");
    return 0;
}

// DLL は C リンケージで公開（inject 後の GetProcAddress 用 / デバッグ用）
extern "C" __declspec(dllexport) DWORD WINAPI MinecraftAIStart() {
    if (!g_running) {
        g_running = true;
        g_thread = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
        if (g_thread) CloseHandle(g_thread);
    }
    return 0;
}

extern "C" __declspec(dllexport) void WINAPI MinecraftAIStop() {
    g_running = false;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        ResolveDllPaths(hModule, g_dllDir, g_logPath);
        MinecraftAIStart();
    } else if (reason == DLL_PROCESS_DETACH) {
        g_running = false;
    }
    return TRUE;
}
