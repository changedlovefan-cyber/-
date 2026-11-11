#include "input_sim.h"
#include <algorithm>
#include <cctype>

static WORD KeyNameToVk(const std::string& raw) {
    std::string s = raw;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)toupper(c); });
    if (s == "W") return 'W';
    if (s == "A") return 'A';
    if (s == "S") return 'S';
    if (s == "D") return 'D';
    if (s == "SPACE") return VK_SPACE;
    if (s == "SHIFT") return VK_SHIFT;
    if (s == "CTRL") return VK_CONTROL;
    if (s == "E") return 'E';
    if (s == "Q") return 'Q';
    if (s == "F") return 'F';
    if (s == "R") return 'R';
    if (s == "ESC") return VK_ESCAPE;
    if (s == "TAB") return VK_TAB;
    if (s == "ENTER") return VK_RETURN;
    if (s.size() == 1 && s[0] >= '1' && s[0] <= '9') return s[0];
    if (s.size() == 1 && s[0] >= 'A' && s[0] <= 'Z') return s[0];
    return 0;
}

static void SendKey(WORD vk, bool down) {
    if (!vk) return;
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(in));
}

// マウス移動は MOUSEEVENTF_MOVE（絶対座標）ではなく、ゲーム側が相対移動として
// 解釈する必要があるので MOUSEEVENTF_MOVE を 1 ピクセルずつ送る方式は不安定。
// Bedrock(GDK) は Raw Input を切っている場合のみ SendInput の相対移動を拾うため、
// 「Options > Controls > Mouse Settings > Raw Input を OFF」を README で案内している。
// ここでは小数の余りを蓄積して 1px 未満の移動を取りこぼさないようにする。
static void SendMouseMove(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    in.mi.dx = dx;
    in.mi.dy = dy;
    SendInput(1, &in, sizeof(in));
}

static void SendClick(bool left) {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = left ? (MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP) : (MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP);
    SendInput(1, &in, sizeof(in));
}

void ExecuteAction(const AIAction& a) {
    // マウス視点
    SendMouseMove(a.mouseDx, a.mouseDy);
    // クリック
    if (a.clickLeft) SendClick(true);
    if (a.clickRight) SendClick(false);
    // キー押下
    std::vector<WORD> vks;
    for (auto& k : a.keys) {
        WORD vk = KeyNameToVk(k);
        if (vk) vks.push_back(vk);
    }
    for (WORD vk : vks) SendKey(vk, true);
    int dur = a.durationMs;
    if (dur < 100) dur = 100;
    if (dur > 3000) dur = 3000;
    Sleep(dur);
    for (WORD vk : vks) SendKey(vk, false);
}

void ReleaseAllKeys() {
    const WORD all[] = {'W','A','S','D','E','Q','F','R', VK_SPACE, VK_SHIFT, VK_CONTROL};
    for (WORD vk : all) SendKey(vk, false);
}
