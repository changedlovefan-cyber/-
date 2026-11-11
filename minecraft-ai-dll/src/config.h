#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cctype>

// config.json を DLL と同じフォルダから読む簡易設定
struct Config {
    std::wstring ollamaHost = L"127.0.0.1";
    int ollamaPort = 11434;
    std::string model = "gemma4:e4b"; // gemma4 は 2026-04-02 リリース済み。ollama pull 済みのタグを指定
    std::string goal = "木を集めて作業台を作る。障害物があれば回避する。";
    int intervalMs = 1200;      // 1回のAI判断ごとの間隔
    int captureWidth = 800;     // Ollamaに送る画像の幅。gemma4 vision は 640 だと小さすぎることがある
    bool enabledByDefault = true;
    std::string ollamaUrl() const {
        return "http://" + std::string(ollamaHost.begin(), ollamaHost.end()) + ":" + std::to_string(ollamaPort);
    }
};

inline std::string JsonGetString(const std::string& json, const std::string& key, const std::string& def) {
    std::string pat = "\"" + key + "\"";
    size_t p = json.find(pat);
    if (p == std::string::npos) return def;
    p = json.find(':', p + pat.size());
    if (p == std::string::npos) return def;
    p = json.find('"', p);
    if (p == std::string::npos) return def;
    size_t e = json.find('"', p + 1);
    if (e == std::string::npos) return def;
    return json.substr(p + 1, e - p - 1);
}
inline int JsonGetInt(const std::string& json, const std::string& key, int def) {
    std::string pat = "\"" + key + "\"";
    size_t p = json.find(pat);
    if (p == std::string::npos) return def;
    p = json.find(':', p + pat.size());
    if (p == std::string::npos) return def;
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\n' || json[p] == '\r')) p++;
    size_t e = p;
    if (e < json.size() && (json[e] == '-' || json[e] == '+')) e++;
    while (e < json.size() && isdigit((unsigned char)json[e])) e++;
    if (e == p) return def;
    try { return std::stoi(json.substr(p, e - p)); } catch (...) { return def; }
}
inline bool JsonGetBool(const std::string& json, const std::string& key, bool def) {
    std::string pat = "\"" + key + "\"";
    size_t p = json.find(pat);
    if (p == std::string::npos) return def;
    p = json.find(':', p + pat.size());
    if (p == std::string::npos) return def;
    while (p < json.size() && isspace((unsigned char)json[p])) p++;
    if (json.compare(p, 4, "true") == 0) return true;
    if (json.compare(p, 5, "false") == 0) return false;
    return def;
}

inline Config LoadConfig(const std::wstring& dllPath) {
    Config c;
    // dllPath はフルパス。同じフォルダの config.json を読む
    std::wstring::size_type pos = dllPath.find_last_of(L"\\/");
    std::wstring dir = (pos == std::wstring::npos) ? L"." : dllPath.substr(0, pos + 1);
    std::wstring cfgPath = dir + L"config.json";
    HANDLE h = CreateFileW(cfgPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return c;
    char buf[8192] = {};
    DWORD read = 0;
    ReadFile(h, buf, sizeof(buf) - 1, &read, nullptr);
    CloseHandle(h);
    std::string json(buf, read);
    c.model = JsonGetString(json, "model", c.model);
    c.goal = JsonGetString(json, "goal", c.goal);
    c.intervalMs = JsonGetInt(json, "interval_ms", c.intervalMs);
    c.captureWidth = JsonGetInt(json, "capture_width", c.captureWidth);
    c.ollamaPort = JsonGetInt(json, "ollama_port", c.ollamaPort);
    std::string host = JsonGetString(json, "ollama_host", "127.0.0.1");
    c.ollamaHost.assign(host.begin(), host.end());
    c.enabledByDefault = JsonGetBool(json, "enabled_by_default", true);
    return c;
}
