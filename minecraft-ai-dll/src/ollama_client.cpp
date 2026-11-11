#include "ollama_client.h"
#include <objbase.h>
#include <winhttp.h>
#include <algorithm>
#include <cctype>
#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#endif

static std::string BuildPrompt(const std::string& goal) {
    // gemma4 は format に JSON Schema を渡すと grammar 制御で確実に JSON になる。
    // それでもプロンプト側で形式を明示しておく（フォールバック用）。
    return
        "あなたは Minecraft 統合版を操作する AI です。スクリーンショットを見て、次の操作を JSON だけで返してください。説明文は禁止です。\n"
        "目標: " + goal + "\n"
        "返すべき JSON 形式:\n"
        "{\"keys\":[\"W\"],\"mouse_dx\":0,\"mouse_dy\":0,\"click_left\":false,\"click_right\":false,\"duration_ms\":600,\"reason\":\"短い理由\"}\n"
        "ルール:\n"
        "- keys は同時押し可能。使えるキー: W A S D SPACE SHIFT CTRL E Q F R 1-9 ESC\n"
        "- mouse_dx/dy は視点回転。右を向くなら正、左なら負。大きくても 200 以内。\n"
        "- click_left はブロックを掘る/置く/攻撃。click_right は右クリック。\n"
        "- duration_ms は 100〜2000。進むなら W を 600〜1000。\n"
        "- 何もしないなら keys は空配列。\n"
        "- 必ず JSON のみを返すこと。コードブロック ``` も付けないこと。";
}

static std::string JsonEscape(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') o += "\\r";
        else if (c == '\t') o += "\\t";
        else o += c;
    }
    return o;
}

// Ollama /api/chat への POST。レスポンス全文を返す
static bool PostChat(const Config& cfg, const std::string& base64Image, std::string& outBody) {
    // WinHttp は WinHTTP セッション内でスレッドプールを使うが、念のため COM も初期化。
    // (WIC と同じスレッドで使うため、二重初期化は RPC_E_CHANGED_MODE で無害)
    struct ComScope {
        HRESULT hr;
        ComScope() { hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
        ~ComScope() { if (SUCCEEDED(hr)) CoUninitialize(); }
    } comScope;

    HINTERNET hSession = WinHttpOpen(L"MinecraftAI/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    // vision 推論は 10〜30 秒かかることがあるのでタイムアウトを長めに
    WinHttpSetTimeouts(hSession, 5000, 10000, 30000, 120000);
    HINTERNET hConnect = WinHttpConnect(hSession, cfg.ollamaHost.c_str(), (INTERNET_PORT)cfg.ollamaPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    HINTERNET hReq = WinHttpOpenRequest(hConnect, L"POST", L"/api/chat", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hReq) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    // Ollama chat 形式 (vision 対応モデル)
    // - "think": false … gemma4 の thinking モードを切る。ON だと JSON が reasoning_content 側に流れる
    // - "format": JSON Schema … Ollama 0.5+ の grammar 制御。 "json" 文字列より確実
    std::string prompt = BuildPrompt(cfg.goal);
    std::string schema =
        "{\"type\":\"object\","
        "\"properties\":{"
        "\"keys\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}},"
        "\"mouse_dx\":{\"type\":\"integer\"},"
        "\"mouse_dy\":{\"type\":\"integer\"},"
        "\"click_left\":{\"type\":\"boolean\"},"
        "\"click_right\":{\"type\":\"boolean\"},"
        "\"duration_ms\":{\"type\":\"integer\"},"
        "\"reason\":{\"type\":\"string\"}},"
        "\"required\":[\"keys\",\"mouse_dx\",\"mouse_dy\",\"click_left\",\"click_right\",\"duration_ms\",\"reason\"]}";
    std::string body =
        "{\"model\":\"" + JsonEscape(cfg.model) + "\","
        "\"stream\":false,"
        "\"think\":false,"
        "\"format\":" + schema + ","
        "\"options\":{\"temperature\":0.1,\"num_predict\":256},"
        "\"messages\":[{\"role\":\"user\",\"content\":\"" + JsonEscape(prompt) + "\",\"images\":[\"" + base64Image + "\"]}]}";

    std::wstring headers = L"Content-Type: application/json";
    BOOL ok = WinHttpSendRequest(hReq, headers.c_str(), -1, (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (!ok) { WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }
    ok = WinHttpReceiveResponse(hReq, nullptr);
    if (!ok) { WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    DWORD status = 0, sz = sizeof(status);
    WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &sz, nullptr);
    if (status != 200) { WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    outBody.clear();
    char buf[8192];
    DWORD read = 0;
    while (WinHttpReadData(hReq, buf, sizeof(buf), &read) && read > 0) {
        outBody.append(buf, read);
    }
    WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
    return !outBody.empty();
}

// Ollama レスポンスから content を取り出す: {"message":{"content":"{...}"}}
static std::string ExtractContent(const std::string& body) {
    // エラー時は {"error":"..."} が返る
    if (body.find("\"error\"") != std::string::npos) return "";
    std::string key = "\"content\"";
    size_t p = body.find(key);
    if (p == std::string::npos) return body;
    p = body.find(':', p + key.size());
    if (p == std::string::npos) return body;
    p = body.find('"', p);
    if (p == std::string::npos) return body;
    std::string out;
    for (size_t i = p + 1; i < body.size(); i++) {
        char c = body[i];
        if (c == '\\' && i + 1 < body.size()) {
            char n = body[++i];
            if (n == 'n') out += '\n';
            else if (n == 't') out += '\t';
            else if (n == 'r') out += '\r';
            else out += n;
        } else if (c == '"') break;
        else out += c;
    }
    return out;
}

static std::vector<std::string> ParseKeys(const std::string& json) {
    std::vector<std::string> keys;
    size_t p = json.find("\"keys\"");
    if (p == std::string::npos) return keys;
    size_t s = json.find('[', p);
    size_t e = json.find(']', s);
    if (s == std::string::npos || e == std::string::npos) return keys;
    std::string inside = json.substr(s + 1, e - s - 1);
    size_t i = 0;
    while (i < inside.size()) {
        size_t q1 = inside.find('"', i);
        if (q1 == std::string::npos) break;
        size_t q2 = inside.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        std::string k = inside.substr(q1 + 1, q2 - q1 - 1);
        std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c){ return (char)toupper(c); });
        if (!k.empty()) keys.push_back(k);
        i = q2 + 1;
    }
    return keys;
}

bool AskGemma(const std::string& base64Image, const Config& cfg, AIAction& out, std::string& rawResponse) {
    std::string body;
    if (!PostChat(cfg, base64Image, body)) return false;
    rawResponse = body;
    std::string content = ExtractContent(body);
    if (content.empty()) return false;
    size_t s = content.find('{');
    size_t e = content.rfind('}');
    if (s == std::string::npos || e == std::string::npos || e <= s) return false;
    std::string json = content.substr(s, e - s + 1);

    out.keys = ParseKeys(json);
    out.mouseDx = JsonGetInt(json, "mouse_dx", 0);
    out.mouseDy = JsonGetInt(json, "mouse_dy", 0);
    out.clickLeft = JsonGetBool(json, "click_left", false);
    out.clickRight = JsonGetBool(json, "click_right", false);
    out.durationMs = JsonGetInt(json, "duration_ms", 600);
    out.reason = JsonGetString(json, "reason", "");
    if (out.mouseDx > 200) out.mouseDx = 200;
    if (out.mouseDx < -200) out.mouseDx = -200;
    if (out.mouseDy > 200) out.mouseDy = 200;
    if (out.mouseDy < -200) out.mouseDy = -200;
    return true;
}
